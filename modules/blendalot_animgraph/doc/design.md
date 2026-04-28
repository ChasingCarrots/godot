# AnimationGraph

## Animation and Animation Data

For Godot an Animation has multiple tracks where each Track is of a specific type such as "Position", "Rotation", "
Method Call", "Audio Playback", etc. Each Track is associated with a node path on which the value of a sampled Track
acts.

Animation Data represents a sampled animation for a given time. For each track in an animation it contains the sampled
values, e.g. a Vector3 for a position, a Quaternion for a rotation, a function name and its parameters, etc.

A skeletal animation in Godot is specified by a set of animation Tracks that influence the position, rotation, and/or
scale of a bone in a skeleton. A pose can be obtained by sampling all tracks to get the local bone transforms that then
have to be applied on the skeleton.

## Animation Blending

Animation blending combines two or more Animation Data objects to create to create a newly blended Animation Data.
Common animation blending performs a linear blend from Animation Data `A` to Animation Data `B` as specified by a weight
factor `w`. For `w = 0.0` the result is `A` and for `w = 0.5` the resulting Animation Data is halfway between `A` and
`B`. For floating point value animation Tracks or 3D position animation Tracks this is straight forward, for a method
Track one has to define what a blend actually means.

### Synchronized or Phase-Space-Warped Animation Blending

Blending two poses `A` and `B` can be done use linear interpolation on the bone transformations (translation, rotation,
scale). However, when blending a humanoid walk and a run animation this generally does not work well as the input poses
must semantically match to produce sensible result. If input `A` has the left foot on the ground and `B` the right foot
the resulting pose is confusing at best.

A solution to this problem is presented by Bobby Anguelov at https://www.youtube.com/watch?v=Jkv0pbp0ckQ&t=7998s.

In short: animations are using a "SyncTrack" to annotate the periods (or phases) of an animation (e.g. `left foot down`,
`right foot cross`, `right foot down`, `left foot cross`). Two animations that are blended must have the same order of
these periods, however each animation may have different durations for these phases. SyncTracks can then be blended and
used to determine for each animation the actual sampling time.

#### Example

Given two animations:

* `Walk` with SyncTrack
    * `left foot down` [0.0, 1.2], duration = `1.2`
    * `right foot down` [1.2, 2.4], duration = `1.2`
* `ZombieWalk` with SyncTrack
    * `left foot down` [0.0, 1.8], duration = `1.8`
    * `right foot down` [1.8, 1.9], duration = `0.1`

Then blending these two animations, e.g. with a blend weight `w=0.2` we obtain the following SyncTrack:

* `left foot down` [0.0, 1.32] (`1.32 = (1-w) * 1.2 + w * 1.8`), duration = 1.32
* `right foot down` [1.32, 2.3] (`2.3 = (1-w) * 2.4 + w * 1.9`), duration = 0.98

A blend factor of `w=0.0` results in the `Walk` animation's SyncTrack, similarly `w=1.0` results in the `ZombieWalk`
SyncTrack.

Time progresses normally in the blended SyncTrack. However, the resulting time is then interpreted relative to the
phases of the SyncTrack. E.g. for the blended SyncTrack with `w=0.2`, a time at 1.5 would be in the `right foot down`
phase with a relative progression of `(1.5-1.32)/0.98 =~ 0.18 = 18%` through the `right foot down` phase.

When sampling the `Walk` or the `ZombieWalk` animation the relative phase progression is used to sample the inputs. For
each animation we sample at 18% of the `right foot down` phase. For the `Walk` animation we would sample at
`1.2 + 1.2*0.18 = 1.416 ~= 1.42` and for the `ZombieWalk` we sample at `1.8 + 0.1*0.18 = 1.818 ~= 1.82`.

What is crucial to note here is that for synchronized blending one first has to have the blended SyncTrack, progress
time there and then use the relative phase progression when sampling the actual animation that serve as input for the
blend operation.

## Blend Trees

A Blend Tree is a directed acyclic graph consisting of nodes with ports and connections. Input ports are on the left
side of a node and output ports on the right. Nodes produce or process "AnimationData" and the connections transport "
AnimationData".

Connections can be represented as spaghetti lines from an output port of node A to an input port of node B. The graph is
acyclic
meaning there must not be a loop (e.g. output of node A never influences an input port of node A). Such a connection is
invalid.

### Example:

```mermaid
flowchart LR
    AnimationB --> TimeScale("TimeScale
    ----
[ ] scale")
AnimationA --> Blend2("Blend2
----
[ ] blend_amount")
TimeScale --> Blend2
Blend2 --> Output
```

A Blend Tree always has a designated output node where the time delta is specified as an input and after the Blend Tree
evaluation of the Blend Tree it can be used to retrieve the result (i.e. Animation Data) of the Blend Tree.

Some nodes have special names in the Blend Tree:

* **Root node** The output node is also called the root node of the Blend Tree.
* **Leaf nodes** These are the nodes that have no inputs. In the example these are the nodes AnimationA and AnimationB.
* **Parent and child node** For two nodes A and B where B is the node that is connected to the Animation Data output
  port of A
  is called the parent node. The Output node (= Root node) has no parent. In the example above the Blend2 node is the
  parent of
  both AnimationA and TimeScale. Conversely, AnimationA and TimeScale are child nodes of the Blend2 node.

### Blend Tree Evaluation Process

#### Ownership of evaluation data (inputs and outputs)

Except for the output node of a Blend Tree the following properties hold:

* all Blend Tree nodes only operate on properties they own and any other data (e.g. inputs and outputs) are specified
  via arguments to `BLTAnimationNode::evaluate(context, inputs, output)` function of the node.

Advantages:

* Simplifies nodes and pushes complexities to the Blend Tree and State Machine class.
* Simplifies testing of nodes
* Resulting API could be exposed to GDScript such that custom nodes could be implemented in GDScript.

Disadvantages:

* Data has to be managed by the Blend Tree => additional bookkeeping there.

#### Evaluation

Evaluation of the Blend Tree happens in multiple phases to ensure we have syncing dependent timing information available
before performing the actual evaluation. Essentially the Blend Tree has to call the following function on all nodes:

1. `ActivateInputs(const Vector<Node> &input_nodes)`: right to left (i.e. from the root node via depth
   first to the leaf nodes)
2. `CalculateSyncTracks(const Vector<const Node> &input_nodes)`: left to right (leaf nodes to root node)
3. `UpdateTime(delta)`: right to left
4. `Evaluate(const Vector<AnimationData> &input_data, AnimationData& output)`: left to right

## State Machines

```plantuml
@startuml

State Idle
State Walk
State Run
State Fall

[*] -right-> Idle
Idle -right-> Walk
Walk -left-> Idle
Walk -right-> Run
Run -left-> Walk
Walk -up-> Fall
'Idle -right-> Fall
Run -up-> Fall

@enduml
```

# Feature Considerations

This section contains design decisions and their tradeoffs on what the animation graphs should support.

## 1. Generalized data connections / Support of math nodes (or non-AnimationNodes in general)

### Description

The connections in the current AnimationTree only support animation values. Node inputs are specified as parameters of
the AnimationTree. E.g. it is not possible to do some math operation on the floating point value that ends up as blend
weight input of a Blend2 node.

We use the term "value data" to distinguish from Animation Data.

### Use case

* Enables animators to add custom math for blend inputs or to adjust other inputs (e.g. LookAt or IK
  targets).
* Together with "Support of multiple output ports" this can be used to add input parameters that affect multiple node
  inputs.

### Effects on the graph topology

* Need to generalize Input and Output Ports to different types instead of only "Animation Data".
* How to evaluate? Two types of subgraphs:
    * a) Data/value inputs (e.g. for blend weights) that have to be evaluated before UpdateConnections. Maybe restrict
      to data that is not animation data dependent?
    * b) Processing nodes, e.g. for extracted bones.

## 2. Support of multiple output ports

### Description

Current AnimationTree nodes have a single designated output port. A node cannot extract a value that then gets used as
input at a later stage in the graph.

**Depends on**: "Generalized data connections".

### Use case

* E.g. extract Bone transform and use in pose modifying nodes.
* Chain IK.

### Effects on graph topology

* Increases Node complexity for handling output ports. Nodes may have the following output ports:
    * AnimOutput
    * AnimOutput + Data
    * Data
      (Data = bool, float, vec3, quat or ...)
* Can a BlendTree emit values?
    * If so: what happens with the output if the BlendTree is used in a State Machine?
        * => Initially: State Machines only emit Animation Data.
* Simplest case:
    * All value data connections are evaluated always before ActivateInputs().
    * BlendTrees (and therefore embedded graphs) cannot emit values.

### Open Issues

1. Unclear when this is actually needed. Using more specific nodes that perform the desired logic
   may be better (
   c.f. https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-blueprint-bone-driven-controller-in-unreal-engine).
   Likely this is not crucial so should be avoided for now.

## 3. Multi-skeleton evaluation

### Description

Allow an animation graph to affect multiple skeletons.

### Use case

Riding on a horse, interaction between two characters.

## 4. Output re-use

### Description

Output of a single node may be used as input of two or more other nodes. One has to consider re-use of Animation Data
and general value data (see "Generalized data connections") separately.

#### Animation Data Output re-use

Animation Data connections are generally more heavy weight than "Generalized data connections". The latter only contain
a small data type such as a Vector3 or Quaternion, whereas Animation Data is a container of those (and possibly more,
e.g. function names and their parameters). So it is desirable to have a minimal number of Animation Data objects
allocated.

```plantuml
@startuml

left to right direction

abstract Output {}
abstract Blend2 {
bool is_synced

float weight()
}

abstract AnimationA {}
abstract TwoBoneIK {}
abstract BoneExtractor {}
abstract AnimationB {}

AnimationA --> Blend2
AnimationB --> Blend2
AnimationB --> BoneExtractor
BoneExtractor --> TwoBoneIK
Blend2 --> TwoBoneIK
TwoBoneIK --> Output

@enduml
```

In this case the Animation Data output of AnimationB is used both in the Blend2 node and the BoneExtractor node. The
extracted bone information is passed into the TwoBoneIK node. For this to work we have to ensure that the output of
AnimationB is freed only when both the Blend2 node and the BoneExtractor have finished their work.

An alternative layout with the same nodes would not suffer from this:

```plantuml
@startuml

left to right direction

abstract Output {}
abstract Blend2 {
bool is_synced

float weight()
}

abstract AnimationA {}
abstract TwoBoneIK {}
abstract BoneExtractor {}
abstract AnimationB {}

AnimationA --> Blend2
BoneExtractor --> Blend2
AnimationB --> BoneExtractor
BoneExtractor --> TwoBoneIK
Blend2 --> TwoBoneIK
TwoBoneIK --> Output

@enduml
```

Here the AnimationData output of BoneExtractor is used by Blend2 and the extracted bone information is still passed to
the TwoBoneIK.

A more complex case is the following setup where Animation Data is used by two nodes:

```plantuml
@startuml

left to right direction

abstract Output {}
abstract Blend2_UpperBody {
bool is_synced

float weight()
}

abstract Blend2_LowerBody {
bool is_synced

float weight()
}


abstract AnimationA {}
abstract AnimationB {}

AnimationA --> Blend2_UpperBody
AnimationB --> Blend2_UpperBody
AnimationB --> Blend2_LowerBody
Blend2_UpperBody --> Blend2_LowerBody

Blend2_LowerBody --> Output

@enduml
```

Here the output of AnimationB is used in both Blend2_UpperBody and Blend2_LowerBody. However, if both blends are synced
the Blend2_UpperBody and Blend2_LowerBody nodes may compute different time values as their SyncTracks differ. Generally
a connection is invalid if the Animation Data of a node gets combined with itself via a different path in the tree.

#### Data value re-use

```plantuml
@startuml

left to right direction

abstract Output {}
abstract Blend2 {
bool is_synced

float weight()
}

abstract AnimationA {}
abstract Vector3Input {}
abstract UnpackVector3 {
x
y
z
}

AnimationA --> Blend2
Vector3Input --> UnpackVector3
UnpackVector3 --> Blend2
Blend2 --> Output

@enduml
```

Here the UnpackVector3 provides access to the individual 3 components of a Vector3 such that e.g. the x value can be
used as an input to a weight node.

Data value re-use is not a problem as the values would be stored directly in the nodes and are not allocated/deallocated
when a node becomes active/deactivated.

### Use case

* Depends on "Generalized data connections" an input that does some computation gets reused in two separate subtrees.

### Decision

Re-use of animation data ports

## 5. Inputs into embedded subgraphs

### Description

An embedded blend tree can receive inputs of the surrounding blend tree. Inputs are animations or - depending on 1. -
also general input values.

### Use case

* Reuse of some blend logic or to move logic of a part of a blend tree into its own node.

### Effects on graph topology

* Great flexibility and possibly reusability.
* Improves logical block building.
* Probably only of bigger use with 1.

### Open issues

* Inputs to embedded state machines?

## Glossary

### Animation Data

Output of an AnimationGraphNode. Contains everything that can be sampled from an animation such as positions, rotations
but also method calls including function name and arguments.
