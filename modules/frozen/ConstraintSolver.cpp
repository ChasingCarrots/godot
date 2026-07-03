#include "ConstraintSolver.h"

#include "ConstraintElement.h"
#include "ConstraintRule.h"
#include "FiniteDomain.h"
#include "VoxelGrid.h"

#include "core/math/basis.h"
#include "core/math/random_pcg.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "core/profiling/profiling.h"
#include "core/templates/a_hash_map.h"
#include "core/templates/local_vector.h"

// Safety net against C++ stack overflow in the recursive search: a recursion
// depth beyond this aborts the solve cleanly instead of crashing the process.
// Generous for topological layouts; the real fix for very large/deep problems is
// an explicit work stack.
static constexpr int CONSTRAINT_RECURSION_DEPTH_LIMIT = 4096;

// ---------------------------------------------------------------------------
// Tier-B internal machinery. None of this is registered with ClassDB; it is
// plain C++ built from the Tier-A resources at the start of solve() and thrown
// away at the end. Naming stays generic (element/value/tag/interface/slot).
// ---------------------------------------------------------------------------
namespace {

struct IfaceInfo {
	int type_id = -1;
	Transform3D anchor;
	bool required = false;
};

struct ElementInfo {
	StringName id;
	float weight = 1.0f;
	Vector<StringName> tags; // original; kept as Vector -- it feeds the GDScript-facing solution/callback
	LocalVector<int> tag_ids; // interned
	LocalVector<int> needed_ids; // interned tags_needed (TagCompatibility)
	LocalVector<int> excluded_ids; // interned excluded_by_tags (TagCompatibility)
	LocalVector<IfaceInfo> interfaces;
	PackedVector3Array geometry;
};

// Each compiled rule carries `rule`: the index of the originating ConstraintRule
// in the problem's rules array, so rejections can be attributed back to the exact
// rule instance a designer authored (for the per-rule statistics).
struct CountRule {
	int tag = -1;
	int mn = 0;
	int mx = -1;
	int rule = -1;
};
struct ReqRule {
	int tag = -1;
	LocalVector<int> req;
	bool as_stack = false;
	int rule = -1;
};
struct ConnRule {
	int tx = -1;
	int ty = -1;
	int rule = -1;
};
struct ConsecRule {
	int tag = -1;
	int mx = 1;
	int rule = -1;
};
struct AdjRule {
	int tag = -1;
	int mx = 1;
	int rule = -1;
};
struct DegRule {
	int tag = -1;
	int mn = 0;
	int mx = -1; // < 0 = unbounded
	int rule = -1;
};
struct NeighRule {
	int tag = -1;
	LocalVector<int> allowed; // interned tag ids permitted as neighbors
	int rule = -1;
};
struct RepeatRule {
	int value = -1; // catalog value index, or -1 = applies to every id
	int mx = -1; // < 0 = unbounded
	int rule = -1;
};
struct NoRepeatRule {
	int rule = -1;
};
struct LeafRule {
	int tag = -1; // interned tag id, or -1 = any
	int mn = 0;
	int mx = -1; // < 0 = unbounded
	int rule = -1;
};
// TagReachability: `required` must all share one connected component, where an edge
// joins any two link tags (name begins with `prefix`) co-carried by a placed element.
struct ReachRule {
	String prefix; // link_prefix as String (for begins_with)
	LocalVector<int> required; // interned required tag ids
	LocalVector<uint8_t> link_mask; // sized num_tags; 1 = tag is a graph node
	int rule = -1;
};
struct CbRule {
	Callable callable;
	int rule = -1;
};

// Per-rule statistics gathered during a solve. `kind`/`label` describe the
// authored rule for a readable report; `rejections` counts forward-check vetoes
// (a candidate placement blocked) and `completion_failures` counts finished
// layouts this rule invalidated.
struct RuleStat {
	int kind = 0;
	StringName label;
	int rejections = 0;
	int completion_failures = 0;
};

struct PlacedNode {
	int value = -1;
	Transform3D xform;
	int parent = -1;
	int degree = 0; // number of connected neighbors (parent + children)
	PackedInt32Array voxels;
};

struct OpenIface {
	int node = -1;
	int iface = -1;
};

// Precomputed (value, interface) pair connectable through a given interface type.
// Lets _grow_expand enumerate candidates for an interface by type lookup instead
// of rescanning the whole catalog on every expansion step.
struct IfaceRef {
	int value = -1;
	int iface = -1;
};

// Weighted-random ordering of candidate values (GameAIPro2 §26.9.3): each
// candidate gets key = u^(1/weight) and we visit them in descending key order,
// which is a weighted shuffle.
struct Candidate {
	bool cap = false;
	int value = -1;
	int iface = -1;
	double key = 0.0;
};
struct CandidateSorter {
	bool operator()(const Candidate &a, const Candidate &b) const { return a.key > b.key; }
};

struct SolveState {
	// --- catalog ---
	LocalVector<ElementInfo> elements;
	AHashMap<StringName, int> id_to_value;
	AHashMap<StringName, int> tag_intern;
	LocalVector<StringName> tag_names;
	AHashMap<StringName, int> type_intern;
	int num_values = 0;
	int num_tags = 0;
	LocalVector<FiniteDomain> tagvals; // per tag id: bitset over values carrying it
	AHashMap<int, LocalVector<IfaceRef>> iface_by_type; // type_id -> connectable (value, iface) pairs

	// --- tag-pool compatibility (TagCompatibility) ---
	LocalVector<bool> ambient_has; // sized num_tags; ambient membership in the pool R
	LocalVector<int> excl_demand; // sized num_tags; how many PLACED nodes exclude tag t (GROW)

	// --- compiled rules ---
	LocalVector<CountRule> counts;
	LocalVector<ReqRule> reqs;
	LocalVector<ConnRule> conns;
	LocalVector<ConsecRule> consecs;
	LocalVector<AdjRule> adjs;
	LocalVector<DegRule> degs;
	LocalVector<NeighRule> neighs;
	LocalVector<RepeatRule> repeats;
	LocalVector<NoRepeatRule> no_repeats;
	LocalVector<LeafRule> leaves;
	LocalVector<ReachRule> reaches;
	LocalVector<CbRule> callbacks;
	bool has_compat = false;
	int compat_rule = -1;
	bool has_geom = false;
	AABB gbounds;
	float gvoxel = 1.0f;
	int ggrow = 0;
	bool goob = true;
	int geom_rule = -1;
	Ref<VoxelGrid> grid;

	// --- search bookkeeping ---
	RandomPCG rng;
	int steps = 0;
	int max_steps = 1000000;
	uint64_t start_ms = 0;
	int time_budget = 0;
	bool aborted = false;
	bool depth_exceeded = false;
	int depth = 0;
	int max_depth = CONSTRAINT_RECURSION_DEPTH_LIMIT;

	// --- statistics ---
	LocalVector<RuleStat> rule_stats; // indexed by original rule position
	int backtracks = 0;
	int candidates_evaluated = 0;
	int candidates_accepted = 0;
	_FORCE_INLINE_ void reject(int p_rule) {
		if (p_rule >= 0 && p_rule < (int)rule_stats.size()) {
			rule_stats[p_rule].rejections++;
		}
	}
	_FORCE_INLINE_ void complete_fail(int p_rule) {
		if (p_rule >= 0 && p_rule < (int)rule_stats.size()) {
			rule_stats[p_rule].completion_failures++;
		}
	}

	// --- GROW state ---
	LocalVector<PlacedNode> placed;
	LocalVector<OpenIface> frontier;
	LocalVector<int> tag_count;
	LocalVector<int> value_count; // per catalog value: how many placed nodes instance it
	int min_elements = 0;
	int max_elements = -1;

	// --- FIXED state ---
	LocalVector<int> fixed_assignment;

	int intern_tag(const StringName &p_tag) {
		AHashMap<StringName, int>::Iterator it = tag_intern.find(p_tag);
		if (it) {
			return it->value;
		}
		const int id = (int)tag_names.size();
		tag_intern.insert(p_tag, id);
		tag_names.push_back(p_tag);
		return id;
	}
	int intern_type(const StringName &p_type) {
		AHashMap<StringName, int>::Iterator it = type_intern.find(p_type);
		if (it) {
			return it->value;
		}
		const int id = (int)type_intern.size();
		type_intern.insert(p_type, id);
		return id;
	}

	_FORCE_INLINE_ bool value_has_tag(int v, int t) const { return tagvals[t].test((uint32_t)v); }

	// Tag-pool membership: t is in R if it is ambient or carried by any placed node.
	_FORCE_INLINE_ bool pool_has(int t) const { return ambient_has[t] || tag_count[t] > 0; }

	bool budget_exceeded() {
		if (steps > max_steps) {
			return true;
		}
		if (time_budget > 0 && (OS::get_singleton()->get_ticks_msec() - start_ms) > (uint64_t)time_budget) {
			return true;
		}
		return false;
	}

	double weighted_key(float p_weight) {
		const double u = MAX((double)rng.randf(), 1e-6);
		const double w = p_weight > 0.0f ? (double)p_weight : 1e-6;
		return Math::pow(u, 1.0 / w);
	}

	static bool is_subsequence(const LocalVector<int> &p_req, const LocalVector<int> &p_seq) {
		int j = 0;
		for (int i = 0; i < (int)p_seq.size() && j < (int)p_req.size(); i++) {
			if (p_seq[i] == p_req[j]) {
				j++;
			}
		}
		return j == (int)p_req.size();
	}
};

// RAII recursion-depth guard: bumps SolveState::depth for the lifetime of a
// recursive solve frame and flags an abort if the configured limit is reached,
// turning a would-be stack overflow into a clean "depth exceeded" failure.
struct DepthGuard {
	SolveState &st;
	bool ok;
	explicit DepthGuard(SolveState &p_st) :
			st(p_st) {
		st.depth++;
		ok = st.depth <= st.max_depth;
	}
	~DepthGuard() { st.depth--; }
};

} // namespace

// ---------------------------------------------------------------------------
// Catalog / rule compilation
// ---------------------------------------------------------------------------
static void _build_catalog(SolveState &st, const Ref<ConstraintProblem> &p_problem) {
	const Vector<Ref<ConstraintElement>> &els = p_problem->get_elements_vector();
	st.num_values = els.size();
	st.elements.resize((uint32_t)st.num_values);

	for (int v = 0; v < els.size(); v++) {
		const Ref<ConstraintElement> &src = els[v];
		ElementInfo info;
		if (src.is_valid()) {
			info.id = src->get_id();
			info.weight = src->get_weight();
			info.geometry = src->get_geometry_points();
			info.tags = src->get_tags_vector();
			for (const StringName &t : info.tags) {
				info.tag_ids.push_back(st.intern_tag(t));
			}
			for (const StringName &t : src->get_tags_needed_vector()) {
				info.needed_ids.push_back(st.intern_tag(t));
			}
			for (const StringName &t : src->get_excluded_by_tags_vector()) {
				info.excluded_ids.push_back(st.intern_tag(t));
			}
			for (const Ref<ConstraintInterface> &iface : src->get_interfaces_vector()) {
				IfaceInfo ii;
				if (iface.is_valid()) {
					ii.type_id = st.intern_type(iface->get_type());
					ii.anchor = iface->get_anchor();
					ii.required = iface->get_required();
				}
				info.interfaces.push_back(ii);
			}
			if (info.id != StringName()) {
				if (st.id_to_value.has(info.id)) {
					WARN_PRINT("ConstraintSolver: duplicate element id \"" + String(info.id) + "\"; the later element shadows the earlier one.");
				}
				st.id_to_value.insert(info.id, v);
			}
		}
		st.elements[v] = info;
	}

	// Compile rules; intern any tags they reference (may not appear on elements).
	// `ri` is the rule's authored index, recorded on each compiled struct and in
	// rule_stats so statistics attribute back to the exact rule instance.
	const Vector<Ref<ConstraintRule>> &rules = p_problem->get_rules_vector();
	const bool is_fixed = p_problem->get_topology_mode() == ConstraintProblem::TOPOLOGY_FIXED;
	st.rule_stats.resize((uint32_t)rules.size());
	for (int ri = 0; ri < rules.size(); ri++) {
		const Ref<ConstraintRule> &r = rules[ri];
		RuleStat stat;
		if (r.is_null()) {
			st.rule_stats[ri] = stat;
			continue;
		}
		stat.kind = r->get_kind();
		switch (r->get_kind()) {
			case ConstraintRule::KIND_COUNT_BY_TAG: {
				const ConstraintCountByTag *cr = Object::cast_to<ConstraintCountByTag>(r.ptr());
				if (!cr) {
					break;
				}
				CountRule c;
				c.tag = st.intern_tag(cr->get_tag());
				c.mn = cr->get_min_count();
				c.mx = cr->get_max_count();
				c.rule = ri;
				stat.label = cr->get_tag();
				st.counts.push_back(c);
			} break;
			case ConstraintRule::KIND_REQUIRES_TAG_BEFORE: {
				const ConstraintRequiresTagBefore *rr = Object::cast_to<ConstraintRequiresTagBefore>(r.ptr());
				if (!rr) {
					break;
				}
				if (is_fixed) {
					WARN_PRINT("ConstraintSolver: RequiresTagBefore is only enforced in GROW topology; it is ignored in FIXED mode.");
				}
				ReqRule rq;
				rq.tag = st.intern_tag(rr->get_tag());
				rq.as_stack = rr->get_as_stack();
				for (const StringName &t : rr->get_required_before_vector()) {
					rq.req.push_back(st.intern_tag(t));
				}
				rq.rule = ri;
				stat.label = rr->get_tag();
				st.reqs.push_back(rq);
			} break;
			case ConstraintRule::KIND_TAG_CONNECTS_TO_TAG: {
				const ConstraintTagConnectsToTag *tr = Object::cast_to<ConstraintTagConnectsToTag>(r.ptr());
				if (!tr) {
					break;
				}
				ConnRule c;
				c.tx = st.intern_tag(tr->get_tag_x());
				c.ty = st.intern_tag(tr->get_tag_y());
				c.rule = ri;
				stat.label = String(tr->get_tag_x()) + "->" + String(tr->get_tag_y());
				st.conns.push_back(c);
			} break;
			case ConstraintRule::KIND_MAX_CONSECUTIVE_TAG: {
				const ConstraintMaxConsecutiveTag *mr = Object::cast_to<ConstraintMaxConsecutiveTag>(r.ptr());
				if (!mr) {
					break;
				}
				if (is_fixed) {
					WARN_PRINT("ConstraintSolver: MaxConsecutiveTag is only enforced in GROW topology; it is ignored in FIXED mode.");
				}
				ConsecRule c;
				c.tag = st.intern_tag(mr->get_tag());
				c.mx = mr->get_max_consecutive();
				c.rule = ri;
				stat.label = mr->get_tag();
				st.consecs.push_back(c);
			} break;
			case ConstraintRule::KIND_MAX_ADJACENT_BY_TAG: {
				const ConstraintMaxAdjacentByTag *ar = Object::cast_to<ConstraintMaxAdjacentByTag>(r.ptr());
				if (!ar) {
					break;
				}
				AdjRule a;
				a.tag = st.intern_tag(ar->get_tag());
				a.mx = ar->get_max_adjacent();
				a.rule = ri;
				stat.label = ar->get_tag();
				st.adjs.push_back(a);
			} break;
			case ConstraintRule::KIND_CONNECTION_COUNT_BY_TAG: {
				const ConstraintConnectionCountByTag *cc = Object::cast_to<ConstraintConnectionCountByTag>(r.ptr());
				if (!cc) {
					break;
				}
				DegRule d;
				d.tag = st.intern_tag(cc->get_tag());
				d.mn = cc->get_min_connections();
				d.mx = cc->get_max_connections();
				d.rule = ri;
				stat.label = cc->get_tag();
				st.degs.push_back(d);
			} break;
			case ConstraintRule::KIND_NEIGHBOR_TAG_ALLOWED: {
				const ConstraintNeighborTagAllowed *nr = Object::cast_to<ConstraintNeighborTagAllowed>(r.ptr());
				if (!nr) {
					break;
				}
				NeighRule nn;
				nn.tag = st.intern_tag(nr->get_tag());
				for (const StringName &t : nr->get_allowed_neighbor_tags_vector()) {
					nn.allowed.push_back(st.intern_tag(t));
				}
				nn.rule = ri;
				stat.label = nr->get_tag();
				st.neighs.push_back(nn);
			} break;
			case ConstraintRule::KIND_MAX_REPEATS_BY_ELEMENT_ID: {
				const ConstraintMaxRepeatsByElementId *mr = Object::cast_to<ConstraintMaxRepeatsByElementId>(r.ptr());
				if (!mr) {
					break;
				}
				RepeatRule rp;
				const StringName eid = mr->get_element_id();
				if (eid != StringName()) {
					AHashMap<StringName, int>::Iterator vi = st.id_to_value.find(eid);
					rp.value = vi ? vi->value : -2; // -2 = a named id absent from the catalog: matches nothing
				} else {
					rp.value = -1; // applies to every id independently
				}
				rp.mx = mr->get_max_repeats();
				rp.rule = ri;
				stat.label = eid;
				st.repeats.push_back(rp);
			} break;
			case ConstraintRule::KIND_NO_REPEAT_NEIGHBOR_ELEMENT: {
				NoRepeatRule nrp;
				nrp.rule = ri;
				stat.label = StringName("element");
				st.no_repeats.push_back(nrp);
			} break;
			case ConstraintRule::KIND_LEAF_COUNT_BY_TAG: {
				const ConstraintLeafCountByTag *lr = Object::cast_to<ConstraintLeafCountByTag>(r.ptr());
				if (!lr) {
					break;
				}
				LeafRule l;
				const StringName tag = lr->get_tag();
				l.tag = (tag != StringName()) ? st.intern_tag(tag) : -1;
				l.mn = lr->get_min_count();
				l.mx = lr->get_max_count();
				l.rule = ri;
				stat.label = (tag != StringName()) ? tag : StringName("*");
				st.leaves.push_back(l);
			} break;
			case ConstraintRule::KIND_TAG_COMPATIBILITY: {
				st.has_compat = true;
				st.compat_rule = ri;
				stat.label = StringName("compat");
			} break;
			case ConstraintRule::KIND_TAG_REACHABILITY: {
				const ConstraintTagReachability *rr = Object::cast_to<ConstraintTagReachability>(r.ptr());
				if (!rr) {
					break;
				}
				ReachRule rre;
				rre.prefix = String(rr->get_link_prefix());
				for (const StringName &t : rr->get_required_tags_vector()) {
					rre.required.push_back(st.intern_tag(t));
				}
				rre.rule = ri;
				stat.label = StringName("reach");
				st.reaches.push_back(rre);
			} break;
			case ConstraintRule::KIND_GEOMETRY: {
				const ConstraintGeometry *gr = Object::cast_to<ConstraintGeometry>(r.ptr());
				if (!gr) {
					break;
				}
				st.has_geom = true;
				st.gbounds = gr->get_bounds();
				st.gvoxel = gr->get_voxel_size();
				st.ggrow = gr->get_spacing_grow();
				st.goob = gr->get_out_of_bounds_occupied();
				st.geom_rule = ri;
			} break;
			case ConstraintRule::KIND_CALLBACK: {
				const ConstraintCallback *cb = Object::cast_to<ConstraintCallback>(r.ptr());
				if (!cb) {
					break;
				}
				if (is_fixed) {
					WARN_PRINT("ConstraintSolver: Callback rules are only evaluated in GROW topology; they are ignored in FIXED mode.");
				}
				CbRule cbr;
				cbr.callable = cb->get_callable();
				cbr.rule = ri;
				st.callbacks.push_back(cbr);
			} break;
			default:
				break;
		}
		st.rule_stats[ri] = stat;
	}

	// Intern ambient tags before the tag-id space is finalized (cheap; done even
	// without a TagCompatibility rule so the id space is stable).
	LocalVector<int> ambient_ids;
	for (const StringName &t : p_problem->get_ambient_tags_vector()) {
		ambient_ids.push_back(st.intern_tag(t));
	}

	st.num_tags = (int)st.tag_names.size();
	st.tagvals.resize((uint32_t)st.num_tags);
	for (int t = 0; t < st.num_tags; t++) {
		st.tagvals[t].resize_values((uint32_t)st.num_values, false);
	}
	for (int v = 0; v < st.num_values; v++) {
		for (int t : st.elements[v].tag_ids) {
			st.tagvals[t].set((uint32_t)v);
		}
	}

	// ConstraintTagReachability: with the tag-id space final, flag which tags are
	// graph nodes (name begins with link_prefix) for each reachability rule.
	for (ReachRule &rr : st.reaches) {
		rr.link_mask.resize((uint32_t)st.num_tags);
		for (int t = 0; t < st.num_tags; t++) {
			rr.link_mask[t] = (uint8_t)String(st.tag_names[t]).begins_with(rr.prefix);
		}
	}

	// Tag-pool ambient membership, sized to the final tag-id space.
	st.ambient_has.resize((uint32_t)st.num_tags);
	for (int t = 0; t < st.num_tags; t++) {
		st.ambient_has[t] = false;
	}
	for (int t : ambient_ids) {
		st.ambient_has[t] = true;
	}

	// Index every interface by its type so GROW candidate enumeration is a hash
	// lookup rather than a full catalog scan per expansion. Built in ascending
	// (value, iface) order so the rng draw order -- and thus determinism -- matches
	// the previous linear scan.
	for (int v = 0; v < st.num_values; v++) {
		const ElementInfo &e = st.elements[v];
		for (int i = 0; i < (int)e.interfaces.size(); i++) {
			st.iface_by_type[e.interfaces[i].type_id].push_back(IfaceRef{ v, i });
		}
	}

	if (st.has_geom) {
		st.grid.instantiate();
		st.grid->configure_aabb(st.gbounds, st.gvoxel);
		st.grid->set_out_of_bounds_occupied(st.goob);
	}
}

// ---------------------------------------------------------------------------
// TagReachability completion check (shared by GROW and FIXED)
// ---------------------------------------------------------------------------
// Union-find "find" with path halving.
static int _uf_find(LocalVector<int> &p_parent, int p_x) {
	while (p_parent[p_x] != p_x) {
		p_parent[p_x] = p_parent[p_parent[p_x]];
		p_x = p_parent[p_x];
	}
	return p_x;
}

// Validates every ConstraintTagReachability rule against the set of placed catalog
// values (topology-agnostic: it ignores the connection graph). For each rule it
// union-finds the link tags co-carried by each placed element, then requires that
// all of the rule's required tags share one component. Records a completion failure
// and returns false on the first rule not satisfied.
static bool _check_reachability(SolveState &st, const LocalVector<int> &p_values) {
	for (const ReachRule &rr : st.reaches) {
		if (rr.required.size() <= 1) {
			continue; // 0 or 1 required node is trivially a single component
		}
		LocalVector<int> parent;
		parent.resize((uint32_t)st.num_tags);
		for (int i = 0; i < st.num_tags; i++) {
			parent[i] = i;
		}
		for (const int v : p_values) {
			int first = -1;
			for (const int t : st.elements[v].tag_ids) {
				if (!rr.link_mask[t]) {
					continue;
				}
				if (first < 0) {
					first = t;
				} else {
					const int ra = _uf_find(parent, first);
					const int rb = _uf_find(parent, t);
					if (ra != rb) {
						parent[ra] = rb;
					}
				}
			}
		}
		const int root = _uf_find(parent, rr.required[0]);
		for (uint32_t i = 1; i < rr.required.size(); i++) {
			if (_uf_find(parent, rr.required[i]) != root) {
				st.complete_fail(rr.rule);
				return false;
			}
		}
	}
	return true;
}

// ---------------------------------------------------------------------------
// GROW mode: forward-checking expansion + backtracking
// ---------------------------------------------------------------------------
// Returns the index of the rule that rejects this attach, or -1 if all pass.
static int _req_ok_for_attach(SolveState &st, int p_attach_node, int p_value) {
	if (st.reqs.is_empty()) {
		return -1;
	}
	// Ancestor chain of the attachment point, node -> ... -> root.
	LocalVector<int> chain;
	for (int n = p_attach_node; n >= 0; n = st.placed[n].parent) {
		chain.push_back(n);
	}
	for (const ReqRule &r : st.reqs) {
		if (!st.value_has_tag(p_value, r.tag)) {
			continue; // rule does not apply to this element
		}
		if (!r.as_stack) {
			// Set semantics: every required tag must appear somewhere on the path.
			for (int rt : r.req) {
				bool found = false;
				for (int n : chain) {
					if (st.value_has_tag(st.placed[n].value, rt)) {
						found = true;
						break;
					}
				}
				if (!found) {
					return r.rule;
				}
			}
		} else {
			// Stack semantics: required tags must appear in order along the path
			// from root down to the attachment point.
			LocalVector<int> seq;
			for (int i = (int)chain.size() - 1; i >= 0; i--) {
				const int nv = st.placed[chain[i]].value;
				for (int rt : r.req) {
					if (st.value_has_tag(nv, rt)) {
						seq.push_back(rt);
					}
				}
			}
			if (!SolveState::is_subsequence(r.req, seq)) {
				return r.rule;
			}
		}
	}
	return -1;
}

// Forward checks for the single new edge created by attaching `p_value` as a
// child of `p_attach_node`: max-consecutive run, max-adjacent degree, and
// neighbor-tag restriction. Returns the rejecting rule index, or -1 if all pass.
static int _grow_local_rules_ok(SolveState &st, int p_attach_node, int p_value) {
	// MaxConsecutiveTag: longest unbroken run of the tag ending at the new node.
	for (const ConsecRule &r : st.consecs) {
		if (!st.value_has_tag(p_value, r.tag)) {
			continue;
		}
		int run = 1;
		for (int n = p_attach_node; n >= 0 && st.value_has_tag(st.placed[n].value, r.tag); n = st.placed[n].parent) {
			run++;
		}
		if (run > r.mx) {
			return r.rule;
		}
	}
	const int parent_value = st.placed[p_attach_node].value;
	// MaxAdjacentByTag: the new edge raises the parent's degree by one and gives
	// the child a degree of one.
	const int parent_new_degree = st.placed[p_attach_node].degree + 1;
	for (const AdjRule &r : st.adjs) {
		if (st.value_has_tag(parent_value, r.tag) && parent_new_degree > r.mx) {
			return r.rule;
		}
		if (st.value_has_tag(p_value, r.tag) && 1 > r.mx) {
			return r.rule;
		}
	}
	// ConnectionCountByTag: enforce the max bound as a forward check (the min bound
	// is checked at completion, since a node's final degree isn't known mid-grow).
	for (const DegRule &r : st.degs) {
		if (r.mx < 0) {
			continue;
		}
		if (st.value_has_tag(parent_value, r.tag) && parent_new_degree > r.mx) {
			return r.rule;
		}
		if (st.value_has_tag(p_value, r.tag) && 1 > r.mx) {
			return r.rule;
		}
	}
	// NeighborTagAllowed: validate the new parent<->child edge in both directions.
	for (const NeighRule &r : st.neighs) {
		if (st.value_has_tag(parent_value, r.tag)) {
			bool ok = false;
			for (int t : r.allowed) {
				if (st.value_has_tag(p_value, t)) {
					ok = true;
					break;
				}
			}
			if (!ok) {
				return r.rule;
			}
		}
		if (st.value_has_tag(p_value, r.tag)) {
			bool ok = false;
			for (int t : r.allowed) {
				if (st.value_has_tag(parent_value, t)) {
					ok = true;
					break;
				}
			}
			if (!ok) {
				return r.rule;
			}
		}
	}
	// MaxRepeatsByElementId: cap how many times this template may be instanced.
	for (const RepeatRule &r : st.repeats) {
		if (r.mx < 0) {
			continue;
		}
		if (r.value == -1 || r.value == p_value) {
			if (st.value_count[p_value] + 1 > r.mx) {
				return r.rule;
			}
		}
	}
	// NoRepeatNeighborElement: the new edge must not join two identical templates.
	if (!st.no_repeats.is_empty() && p_value == parent_value) {
		return st.no_repeats[0].rule;
	}
	// TagCompatibility (excludes, forward): the candidate must not be excluded by a
	// tag already in the pool, nor carry a tag an already-placed element excludes.
	// (requires is a completion check; a needed tag may still arrive later.)
	if (st.has_compat) {
		const ElementInfo &cand = st.elements[p_value];
		for (int e : cand.excluded_ids) {
			if (st.pool_has(e)) {
				return st.compat_rule;
			}
		}
		for (int t : cand.tag_ids) {
			if (st.excl_demand[t] > 0) {
				return st.compat_rule;
			}
		}
	}
	return -1;
}

// Returns the index of the callback rule that rejects this placement, or -1.
static int _callbacks_ok(SolveState &st, int p_value, const Transform3D &p_xform, int p_parent_node) {
	if (st.callbacks.is_empty()) {
		return -1;
	}
	const ElementInfo &e = st.elements[p_value];
	Dictionary d;
	d["element_id"] = e.id;
	d["transform"] = p_xform;
	TypedArray<StringName> tags;
	for (const StringName &t : e.tags) {
		tags.append(t);
	}
	d["tags"] = tags;
	d["topology"] = StringName("grow");
	if (p_parent_node >= 0) {
		d["parent_id"] = st.elements[st.placed[p_parent_node].value].id;
	} else {
		d["parent_id"] = StringName();
	}
	for (const CbRule &c : st.callbacks) {
		if (!c.callable.is_valid()) {
			continue;
		}
		const Variant r = c.callable.call(d);
		if (!(bool)r) {
			return c.rule;
		}
	}
	return -1;
}

static bool _grow_complete(SolveState &st) {
	if ((int)st.placed.size() < st.min_elements) {
		return false;
	}
	// TagCompatibility (requires): the pool is final now, so every placed node's
	// tags_needed must be satisfied by it. (excludes was forward-checked at attach.)
	if (st.has_compat) {
		for (int i = 0; i < (int)st.placed.size(); i++) {
			for (int t : st.elements[st.placed[i].value].needed_ids) {
				if (!st.pool_has(t)) {
					st.complete_fail(st.compat_rule);
					return false;
				}
			}
		}
	}
	for (const CountRule &c : st.counts) {
		const int cnt = st.tag_count[c.tag];
		if (cnt < c.mn) {
			st.complete_fail(c.rule);
			return false;
		}
		if (c.mx >= 0 && cnt > c.mx) {
			st.complete_fail(c.rule);
			return false;
		}
	}
	// ConnectionCountByTag: every tagged node's final degree must be within range.
	for (const DegRule &r : st.degs) {
		for (int i = 0; i < (int)st.placed.size(); i++) {
			if (!st.value_has_tag(st.placed[i].value, r.tag)) {
				continue;
			}
			const int deg = st.placed[i].degree;
			if (deg < r.mn || (r.mx >= 0 && deg > r.mx)) {
				st.complete_fail(r.rule);
				return false;
			}
		}
	}
	if (!st.conns.is_empty() || !st.leaves.is_empty()) {
		// Build child adjacency once (reused by the connects-to and leaf checks).
		LocalVector<LocalVector<int>> children;
		children.resize(st.placed.size());
		for (int i = 0; i < (int)st.placed.size(); i++) {
			const int p = st.placed[i].parent;
			if (p >= 0) {
				children[p].push_back(i);
			}
		}
		// LeafCountByTag: a leaf is a node with no children (a dead-end of the tree).
		for (const LeafRule &r : st.leaves) {
			int cnt = 0;
			for (int i = 0; i < (int)st.placed.size(); i++) {
				if (!children[i].is_empty()) {
					continue;
				}
				if (r.tag < 0 || st.value_has_tag(st.placed[i].value, r.tag)) {
					cnt++;
				}
			}
			if (cnt < r.mn || (r.mx >= 0 && cnt > r.mx)) {
				st.complete_fail(r.rule);
				return false;
			}
		}
		for (const ConnRule &cr : st.conns) {
			for (int i = 0; i < (int)st.placed.size(); i++) {
				if (!st.value_has_tag(st.placed[i].value, cr.tx)) {
					continue;
				}
				bool ok = false;
				const int parent = st.placed[i].parent;
				if (parent >= 0 && st.value_has_tag(st.placed[parent].value, cr.ty)) {
					ok = true;
				}
				if (!ok) {
					for (int m : children[i]) {
						if (st.value_has_tag(st.placed[m].value, cr.ty)) {
							ok = true;
							break;
						}
					}
				}
				if (!ok) {
					st.complete_fail(cr.rule);
					return false;
				}
			}
		}
	}
	// TagReachability: all required tags must land in one connected component.
	if (!st.reaches.is_empty()) {
		LocalVector<int> values;
		values.resize(st.placed.size());
		for (uint32_t i = 0; i < st.placed.size(); i++) {
			values[i] = st.placed[i].value;
		}
		if (!_check_reachability(st, values)) {
			return false;
		}
	}
	return true;
}

static bool _grow_expand(SolveState &st) {
	DepthGuard dg(st);
	if (!dg.ok) {
		st.aborted = true;
		st.depth_exceeded = true;
		return false;
	}
	if (st.budget_exceeded()) {
		st.aborted = true;
		return false;
	}
	st.steps++;

	if (st.frontier.is_empty()) {
		return _grow_complete(st);
	}

	// Decide one open interface (depth-first: take the most recent).
	const OpenIface f = st.frontier[st.frontier.size() - 1];
	st.frontier.remove_at(st.frontier.size() - 1);

	const ElementInfo &owner = st.elements[st.placed[f.node].value];
	const IfaceInfo &fi = owner.interfaces[f.iface];

	// Build the candidate set for this interface.
	LocalVector<Candidate> cands;
	if (!fi.required) {
		Candidate cap;
		cap.cap = true;
		cap.key = st.weighted_key(1.0f);
		cands.push_back(cap);
	}
	const bool at_cap = (st.max_elements >= 0 && (int)st.placed.size() >= st.max_elements);
	if (!at_cap) {
		AHashMap<int, LocalVector<IfaceRef>>::Iterator it = st.iface_by_type.find(fi.type_id);
		if (it) {
			for (const IfaceRef &ir : it->value) {
				Candidate c;
				c.value = ir.value;
				c.iface = ir.iface;
				c.key = st.weighted_key(st.elements[ir.value].weight);
				cands.push_back(c);
			}
		}
	}
	cands.sort_custom<CandidateSorter>();

	const Transform3D parent_anchor_world = st.placed[f.node].xform * fi.anchor;
	// 180° flip about Y so a child interface faces opposite its parent's. Constant,
	// so compute it once (function-local static init is thread-safe in C++11).
	static const Transform3D flip = []() {
		Transform3D t;
		t.basis = Basis(Vector3(0, 1, 0), Math::PI);
		return t;
	}();

	for (const Candidate &c : cands) {
		if (c.cap) {
			if (_grow_expand(st)) {
				return true;
			}
			if (st.aborted) {
				break;
			}
			continue;
		}

		st.candidates_evaluated++;

		// Cardinality (max) forward check.
		int over_max_rule = -1;
		for (const CountRule &cr : st.counts) {
			if (cr.mx >= 0 && st.value_has_tag(c.value, cr.tag) && st.tag_count[cr.tag] + 1 > cr.mx) {
				over_max_rule = cr.rule;
				break;
			}
		}
		if (over_max_rule >= 0) {
			st.reject(over_max_rule);
			continue;
		}
		const int req_rule = _req_ok_for_attach(st, f.node, c.value);
		if (req_rule >= 0) {
			st.reject(req_rule);
			continue;
		}
		const int local_rule = _grow_local_rules_ok(st, f.node, c.value);
		if (local_rule >= 0) {
			st.reject(local_rule);
			continue;
		}

		const ElementInfo &e = st.elements[c.value];
		const Transform3D child_world = parent_anchor_world * flip * e.interfaces[c.iface].anchor.affine_inverse();

		PackedInt32Array voxels;
		if (st.grid.is_valid()) {
			voxels = st.grid->voxels_from_points(e.geometry, child_world, st.ggrow);
			if (st.grid->has_overlap_excluding(voxels, st.placed[f.node].voxels)) {
				st.reject(st.geom_rule);
				continue;
			}
		}
		const int cb_rule = _callbacks_ok(st, c.value, child_world, f.node);
		if (cb_rule >= 0) {
			st.reject(cb_rule);
			continue;
		}

		// Commit.
		st.candidates_accepted++;
		if (st.grid.is_valid()) {
			st.grid->mark_occupied(voxels);
		}
		PlacedNode node;
		node.value = c.value;
		node.xform = child_world;
		node.parent = f.node;
		node.degree = 1; // connected to its parent
		node.voxels = voxels;
		st.placed.push_back(node);
		const int child = (int)st.placed.size() - 1;
		st.placed[f.node].degree++;
		for (int t : e.tag_ids) {
			st.tag_count[t]++;
		}
		st.value_count[c.value]++;
		if (st.has_compat) {
			for (int t : e.excluded_ids) {
				st.excl_demand[t]++;
			}
		}
		int pushed = 0;
		for (int i = 0; i < (int)e.interfaces.size(); i++) {
			if (i == c.iface) {
				continue; // consumed by this connection
			}
			OpenIface oi;
			oi.node = child;
			oi.iface = i;
			st.frontier.push_back(oi);
			pushed++;
		}

		if (_grow_expand(st)) {
			return true;
		}

		// Undo.
		st.backtracks++;
		for (int p = 0; p < pushed; p++) {
			st.frontier.remove_at(st.frontier.size() - 1);
		}
		for (int t : e.tag_ids) {
			st.tag_count[t]--;
		}
		st.value_count[c.value]--;
		if (st.has_compat) {
			for (int t : e.excluded_ids) {
				st.excl_demand[t]--;
			}
		}
		st.placed[f.node].degree--;
		st.placed.remove_at(st.placed.size() - 1);
		if (st.grid.is_valid()) {
			st.grid->mark_unoccupied(voxels);
		}
		if (st.aborted) {
			break;
		}
	}

	st.frontier.push_back(f);
	return false;
}

static void _build_grow_solution(SolveState &st, Ref<ConstraintSolution> p_sol) {
	for (int i = 0; i < (int)st.placed.size(); i++) {
		const ElementInfo &e = st.elements[st.placed[i].value];
		p_sol->add_node(e.id, st.placed[i].xform, e.tags);
	}
	for (int i = 0; i < (int)st.placed.size(); i++) {
		if (st.placed[i].parent >= 0) {
			p_sol->add_connection(st.placed[i].parent, i);
		}
	}
}

// ---------------------------------------------------------------------------
// FIXED mode: AC-3-flavoured cardinality narrowing + MRV backtracking
// ---------------------------------------------------------------------------
static bool _fixed_propagate(SolveState &st, LocalVector<FiniteDomain> &doms) {
	bool changed = true;
	while (changed) {
		changed = false;
		for (const CountRule &c : st.counts) {
			const FiniteDomain &tv = st.tagvals[c.tag];
			int definite = 0, possible = 0;
			for (int s = 0; s < (int)doms.size(); s++) {
				if (!doms[s].intersects(tv)) {
					continue;
				}
				possible++;
				if (doms[s].is_unique()) {
					definite++;
				}
			}
			if (c.mx >= 0 && definite > c.mx) {
				return false;
			}
			if (possible < c.mn) {
				return false;
			}
			if (c.mx >= 0 && definite == c.mx) {
				// No more slots may take this tag.
				for (int s = 0; s < (int)doms.size(); s++) {
					if (doms[s].is_unique() && doms[s].intersects(tv)) {
						continue; // already a definite holder
					}
					if (doms[s].intersects(tv)) {
						if (doms[s].subtract(tv)) {
							changed = true;
							if (doms[s].is_empty()) {
								return false;
							}
						}
					}
				}
			}
			if (possible == c.mn && c.mn > 0) {
				// Every slot that can hold the tag must hold it.
				for (int s = 0; s < (int)doms.size(); s++) {
					if (doms[s].intersects(tv)) {
						if (doms[s].intersect_with(tv)) {
							changed = true;
							if (doms[s].is_empty()) {
								return false;
							}
						}
					}
				}
			}
		}
	}
	return true;
}

static bool _fixed_complete(SolveState &st, const LocalVector<FiniteDomain> &doms,
		const PackedInt32Array &p_conn, const LocalVector<Transform3D> &p_slot_xform) {
	const int n = (int)doms.size();
	LocalVector<int> assign;
	assign.resize((uint32_t)n);
	for (int s = 0; s < n; s++) {
		assign[s] = doms[s].first_value();
	}

	// Cardinality final verification.
	for (const CountRule &c : st.counts) {
		int cnt = 0;
		for (int s = 0; s < n; s++) {
			if (st.value_has_tag(assign[s], c.tag)) {
				cnt++;
			}
		}
		if (cnt < c.mn) {
			st.complete_fail(c.rule);
			return false;
		}
		if (c.mx >= 0 && cnt > c.mx) {
			st.complete_fail(c.rule);
			return false;
		}
	}

	// MaxRepeatsByElementId: no template may be instanced more than its cap.
	if (!st.repeats.is_empty()) {
		for (const RepeatRule &r : st.repeats) {
			if (r.mx < 0) {
				continue;
			}
			if (r.value == -1) {
				// Cap every id independently.
				LocalVector<int> per_value;
				per_value.resize((uint32_t)st.num_values);
				for (int i = 0; i < st.num_values; i++) {
					per_value[i] = 0;
				}
				for (int s = 0; s < n; s++) {
					if (++per_value[assign[s]] > r.mx) {
						st.complete_fail(r.rule);
						return false;
					}
				}
			} else if (r.value >= 0) {
				int cnt = 0;
				for (int s = 0; s < n; s++) {
					if (assign[s] == r.value) {
						cnt++;
					}
				}
				if (cnt > r.mx) {
					st.complete_fail(r.rule);
					return false;
				}
			}
		}
	}

	// TagCompatibility: pool R = ambient ∪ all assigned tags; every assigned
	// element's excluded_by_tags must be absent from R and its tags_needed present.
	// Pool-based (not graph-based), so it stands apart from the adjacency rules.
	if (st.has_compat) {
		LocalVector<bool> present(st.ambient_has); // copy (LocalVector copy ctor is explicit)
		for (int s = 0; s < n; s++) {
			for (int t : st.elements[assign[s]].tag_ids) {
				present[t] = true;
			}
		}
		for (int s = 0; s < n; s++) {
			const ElementInfo &e = st.elements[assign[s]];
			for (int x : e.excluded_ids) {
				if (present[x]) {
					st.complete_fail(st.compat_rule);
					return false;
				}
			}
			for (int q : e.needed_ids) {
				if (!present[q]) {
					st.complete_fail(st.compat_rule);
					return false;
				}
			}
		}
	}

	// Adjacency-graph rules over the fixed connection graph.
	if (!st.conns.is_empty() || !st.adjs.is_empty() || !st.degs.is_empty() || !st.neighs.is_empty() ||
			!st.no_repeats.is_empty() || !st.leaves.is_empty()) {
		LocalVector<LocalVector<int>> adj;
		adj.resize((uint32_t)n);
		for (int i = 0; i + 1 < p_conn.size(); i += 2) {
			const int a = p_conn[i];
			const int b = p_conn[i + 1];
			if (a >= 0 && a < n && b >= 0 && b < n) {
				adj[a].push_back(b);
				adj[b].push_back(a);
			}
		}
		// tag-connects-to-tag: each tx element needs at least one ty neighbor.
		for (const ConnRule &cr : st.conns) {
			for (int s = 0; s < n; s++) {
				if (!st.value_has_tag(assign[s], cr.tx)) {
					continue;
				}
				bool ok = false;
				for (int m : adj[s]) {
					if (st.value_has_tag(assign[m], cr.ty)) {
						ok = true;
						break;
					}
				}
				if (!ok) {
					st.complete_fail(cr.rule);
					return false;
				}
			}
		}
		// max-adjacent-by-tag: a tag element's degree must not exceed the limit.
		for (const AdjRule &ar : st.adjs) {
			for (int s = 0; s < n; s++) {
				if (st.value_has_tag(assign[s], ar.tag) && (int)adj[s].size() > ar.mx) {
					st.complete_fail(ar.rule);
					return false;
				}
			}
		}
		// connection-count-by-tag: a tag element's degree must be within range.
		for (const DegRule &dr : st.degs) {
			for (int s = 0; s < n; s++) {
				if (!st.value_has_tag(assign[s], dr.tag)) {
					continue;
				}
				const int deg = (int)adj[s].size();
				if (deg < dr.mn || (dr.mx >= 0 && deg > dr.mx)) {
					st.complete_fail(dr.rule);
					return false;
				}
			}
		}
		// neighbor-tag-allowed: every neighbor of a tag element must carry an
		// allowed tag.
		for (const NeighRule &nr : st.neighs) {
			for (int s = 0; s < n; s++) {
				if (!st.value_has_tag(assign[s], nr.tag)) {
					continue;
				}
				for (int m : adj[s]) {
					bool ok = false;
					for (int t : nr.allowed) {
						if (st.value_has_tag(assign[m], t)) {
							ok = true;
							break;
						}
					}
					if (!ok) {
						st.complete_fail(nr.rule);
						return false;
					}
				}
			}
		}
		// no-repeat-neighbor: no edge may join two identical templates.
		if (!st.no_repeats.is_empty()) {
			for (int i = 0; i + 1 < p_conn.size(); i += 2) {
				const int a = p_conn[i];
				const int b = p_conn[i + 1];
				if (a >= 0 && a < n && b >= 0 && b < n && assign[a] == assign[b]) {
					st.complete_fail(st.no_repeats[0].rule);
					return false;
				}
			}
		}
		// leaf-count-by-tag: count slots whose degree is <= 1 (a dead-end).
		for (const LeafRule &r : st.leaves) {
			int cnt = 0;
			for (int s = 0; s < n; s++) {
				if ((int)adj[s].size() > 1) {
					continue;
				}
				if (r.tag < 0 || st.value_has_tag(assign[s], r.tag)) {
					cnt++;
				}
			}
			if (cnt < r.mn || (r.mx >= 0 && cnt > r.mx)) {
				st.complete_fail(r.rule);
				return false;
			}
		}
	}

	// TagReachability: all required tags must land in one connected component.
	if (!st.reaches.is_empty() && !_check_reachability(st, assign)) {
		return false;
	}

	// Geometry: validate no two placed elements overlap.
	if (st.grid.is_valid()) {
		st.grid->clear();
		for (int s = 0; s < n; s++) {
			const PackedInt32Array vox = st.grid->voxels_from_points(st.elements[assign[s]].geometry, p_slot_xform[s], st.ggrow);
			if (st.grid->has_overlap(vox)) {
				st.complete_fail(st.geom_rule);
				return false;
			}
			st.grid->mark_occupied(vox);
		}
	}

	st.fixed_assignment = assign;
	return true;
}

static bool _fixed_solve(SolveState &st, const LocalVector<FiniteDomain> &doms,
		const PackedInt32Array &p_conn, const LocalVector<Transform3D> &p_slot_xform) {
	DepthGuard dg(st);
	if (!dg.ok) {
		st.aborted = true;
		st.depth_exceeded = true;
		return false;
	}
	if (st.budget_exceeded()) {
		st.aborted = true;
		return false;
	}
	st.steps++;

	// MRV: smallest still-undecided domain (GameAIPro2 §26.9.4).
	int best = -1;
	uint32_t best_count = 0xFFFFFFFFu;
	for (int s = 0; s < (int)doms.size(); s++) {
		const uint32_t c = doms[s].count();
		if (c == 0) {
			return false;
		}
		if (c > 1 && c < best_count) {
			best = s;
			best_count = c;
		}
	}
	if (best == -1) {
		return _fixed_complete(st, doms, p_conn, p_slot_xform);
	}

	LocalVector<int> values;
	doms[best].collect_values(values);
	LocalVector<Candidate> ordered;
	for (uint32_t i = 0; i < values.size(); i++) {
		Candidate c;
		c.value = values[i];
		c.key = st.weighted_key(st.elements[values[i]].weight);
		ordered.push_back(c);
	}
	ordered.sort_custom<CandidateSorter>();

	for (const Candidate &c : ordered) {
		LocalVector<FiniteDomain> next(doms);
		next[best].clear();
		next[best].set((uint32_t)c.value);
		if (!_fixed_propagate(st, next)) {
			continue;
		}
		if (_fixed_solve(st, next, p_conn, p_slot_xform)) {
			return true;
		}
		if (st.aborted) {
			return false;
		}
	}
	return false;
}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------
Ref<ConstraintSolution> ConstraintSolver::solve(const Ref<ConstraintProblem> &p_problem, int p_seed_override) {
	GodotProfileFunction();

	Ref<ConstraintSolution> sol;
	sol.instantiate();

	if (p_problem.is_null()) {
		sol->set_failure_reason("Problem is null.");
		return sol;
	}

	SolveState st;
	st.max_steps = p_problem->get_max_steps();
	st.time_budget = p_problem->get_time_budget_ms();
	st.start_ms = OS::get_singleton()->get_ticks_msec();
	const int seed = p_seed_override >= 0 ? p_seed_override : p_problem->get_seed();
	st.rng.seed((uint64_t)seed);

	_build_catalog(st, p_problem);
	st.tag_count.resize((uint32_t)st.num_tags);
	for (int i = 0; i < st.num_tags; i++) {
		st.tag_count[i] = 0;
	}
	st.value_count.resize((uint32_t)st.num_values);
	for (int i = 0; i < st.num_values; i++) {
		st.value_count[i] = 0;
	}
	st.excl_demand.resize((uint32_t)st.num_tags);
	for (int i = 0; i < st.num_tags; i++) {
		st.excl_demand[i] = 0;
	}

	if (st.num_values == 0) {
		sol->set_failure_reason("Problem has no elements.");
		return sol;
	}

	bool ok = false;

	if (p_problem->get_topology_mode() == ConstraintProblem::TOPOLOGY_GROW) {
		st.min_elements = p_problem->get_min_elements();
		st.max_elements = p_problem->get_max_elements();

		AHashMap<StringName, int>::Iterator it = st.id_to_value.find(p_problem->get_start_element());
		if (!it) {
			sol->set_failure_reason("start_element not found in catalog.");
			sol->set_steps(st.steps);
			return sol;
		}
		const int start_value = it->value;
		const ElementInfo &se = st.elements[start_value];

		PlacedNode root;
		root.value = start_value;
		root.xform = Transform3D();
		root.parent = -1;
		if (st.grid.is_valid()) {
			root.voxels = st.grid->voxels_from_points(se.geometry, root.xform, st.ggrow);
			st.grid->mark_occupied(root.voxels);
		}
		st.placed.push_back(root);
		for (int t : se.tag_ids) {
			st.tag_count[t]++;
		}
		st.value_count[start_value]++;
		bool start_infeasible = false;
		if (st.has_compat) {
			for (int t : se.excluded_ids) {
				st.excl_demand[t]++;
			}
			// The root is forced, so an excludes conflict against the ambient pool
			// (or its own tags) makes the whole problem infeasible up front.
			for (int e : se.excluded_ids) {
				if (st.pool_has(e)) {
					start_infeasible = true;
					break;
				}
			}
		}
		for (int i = 0; i < (int)se.interfaces.size(); i++) {
			OpenIface oi;
			oi.node = 0;
			oi.iface = i;
			st.frontier.push_back(oi);
		}

		if (start_infeasible) {
			st.complete_fail(st.compat_rule);
			ok = false;
		} else {
			ok = _grow_expand(st);
			if (ok) {
				_build_grow_solution(st, sol);
			}
		}
	} else {
		// TOPOLOGY_FIXED.
		const Vector<Ref<ConstraintSlot>> &slots = p_problem->get_slots_vector();
		const int n = slots.size();
		LocalVector<FiniteDomain> doms;
		LocalVector<Transform3D> slot_xform;
		doms.resize((uint32_t)n);
		slot_xform.resize((uint32_t)n);
		FiniteDomain all;
		all.resize_values((uint32_t)st.num_values, true);
		for (int s = 0; s < n; s++) {
			slot_xform[s] = slots[s].is_valid() ? slots[s]->get_transform() : Transform3D();
			FiniteDomain d = all;
			if (slots[s].is_valid()) {
				const Vector<StringName> &allowed = slots[s]->get_allowed_tags_vector();
				if (!allowed.is_empty()) {
					// Restrict to elements carrying at least one allowed tag.
					FiniteDomain mask;
					mask.resize_values((uint32_t)st.num_values, false);
					for (const StringName &t : allowed) {
						AHashMap<StringName, int>::Iterator ti = st.tag_intern.find(t);
						if (ti) {
							for (int v = 0; v < st.num_values; v++) {
								if (st.tagvals[ti->value].test((uint32_t)v)) {
									mask.set((uint32_t)v);
								}
							}
						}
					}
					d.intersect_with(mask);
				}
			}
			doms[s] = d;
		}

		const PackedInt32Array conn = p_problem->get_connections();
		if (n == 0) {
			ok = true; // trivially solved
		} else if (_fixed_propagate(st, doms)) {
			ok = _fixed_solve(st, doms, conn, slot_xform);
		}

		if (ok) {
			for (int s = 0; s < n; s++) {
				const ElementInfo &e = st.elements[st.fixed_assignment[s]];
				sol->add_node(e.id, slot_xform[s], e.tags);
			}
			for (int i = 0; i + 1 < conn.size(); i += 2) {
				const int a = conn[i];
				const int b = conn[i + 1];
				if (a >= 0 && a < n && b >= 0 && b < n) {
					sol->add_connection(a, b);
				}
			}
		}
	}

	sol->set_success(ok);
	sol->set_steps(st.steps);
	sol->set_search_stats(st.backtracks, st.candidates_evaluated, st.candidates_accepted);
	for (int i = 0; i < (int)st.rule_stats.size(); i++) {
		const RuleStat &rs = st.rule_stats[i];
		sol->add_rule_stat(i, rs.kind, rs.label, rs.rejections, rs.completion_failures);
	}
	if (!ok) {
		if (st.depth_exceeded) {
			sol->set_failure_reason("Search aborted: recursion depth limit reached (problem too deep; reduce max_elements or raise CONSTRAINT_RECURSION_DEPTH_LIMIT).");
		} else if (st.aborted) {
			sol->set_failure_reason("Search budget exceeded (max_steps/time_budget_ms).");
		} else {
			sol->set_failure_reason("No solution satisfies the constraints.");
		}
	}
	return sol;
}

void ConstraintSolver::_bind_methods() {
	ClassDB::bind_method(D_METHOD("solve", "problem", "seed_override"), &ConstraintSolver::solve, DEFVAL(-1));
}
