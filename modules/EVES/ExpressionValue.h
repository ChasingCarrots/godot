#ifndef EVES_EXPRESSIONVALUE_H
#define EVES_EXPRESSIONVALUE_H

#include "ValueCollection.h"
#include <tinyexpr.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <core/object/class_db.h>
#include <core/object/object.h>

namespace eves {

class Expression {
public:
	Expression &operator=(const Expression &) = delete;
	Expression(Expression const &) = delete;

	Expression(const std::string &expression, const std::vector<std::string> &variables, const std::vector<double> &defaultValues) {
		int numVars = static_cast<int>(variables.size());
		_variableStorage.resize(numVars);
		_exprVariables.resize(numVars);
		for (int i = 0; i < numVars; ++i) {
			_variableStorage[i].Name = variables[i];
			_variableStorage[i].DefaultValue = defaultValues[i];
			_variableStorage[i].CurrentValue = defaultValues[i];
			_exprVariables[i].type = TE_VARIABLE;
			_exprVariables[i].name = _variableStorage[i].Name.c_str();
			_exprVariables[i].address = &_variableStorage[i].CurrentValue;
		}
		int error = 0;
		_expr = te_compile(expression.c_str(), _exprVariables.data(), numVars, &error);
		_exprString = expression;
	}

	~Expression() {
		if (_expr)
			te_free(_expr);
	}

	void ResetVariablesToDefault() {
		for (auto &var : _variableStorage)
			var.CurrentValue = var.DefaultValue;
	}
	auto begin() { return _variableStorage.begin(); }
	auto end() { return _variableStorage.end(); }

	double Evaluate() {
		if (!_expr)
			return 0;
		return te_eval(_expr);
	}

private:
	struct Variable {
		std::string Name;
		double DefaultValue;
		double CurrentValue;
	};
	std::vector<Variable> _variableStorage;
	std::vector<te_variable> _exprVariables;
	te_expr *_expr;
	// only needed for debugging purposes...
	std::string _exprString;
};

// TODO: the ExpressionValue should not be based on ValueCollections!
// (but maybe there is some value in this code, so let's keep it for now)
// class ExpressionValue : public BaseValue {
// public:
// 	explicit ExpressionValue(Expression *expression) {
// 		_expression = expression;
// 	}
//
// 	void AddSingleNamedValue(const std::string &name, BaseValue *value, bool removeAfterEval) {
// 		_singleNamedValues[name] = {
// 			value,
// 			removeAfterEval
// 		};
// 		if (!removeAfterEval) {
// 			_autoDestroyConnections.emplace_back(
// 					value->OnValueChanged.connect([this](eves::BaseValue *) {
// 						SetValueDirty();
// 					}));
// 			_autoDestroyConnections.emplace_back(
// 					value->OnValueDeleted.connect([this](eves::BaseValue *value) {
// 						auto singleValueIter = _singleNamedValues.begin();
// 						while (singleValueIter != _singleNamedValues.end()) {
// 							if (singleValueIter->second.Value == value)
// 								singleValueIter = _singleNamedValues.erase(singleValueIter);
// 							else
// 								++singleValueIter;
// 						}
// 					}));
// 		}
// 		SetValueDirty();
// 	}
//
// 	void AddValueCollection(ValueCollection *collection, const std::string &prefix, bool removeAfterEval) {
// 		_valueCollections.push_back({ prefix,
// 				collection,
// 				removeAfterEval });
// 		if (!removeAfterEval) {
// 			bool hasPrefix = !prefix.empty();
// 			_autoDestroyConnections.emplace_back(
// 					collection->OnValueChanged.connect([this, hasPrefix](const std::string &changedValueName, eves::BaseValue *) {
// 						// when this collection has a prefix, we simply set the dirty flag...
// 						if (hasPrefix) {
// 							SetValueDirty();
// 							return;
// 						}
// 						// otherwise we quickly check if this changed value is part of
// 						// our expression.
// 						if (!_expression)
// 							return;
// 						for (const auto &exprVar : *_expression) {
// 							if (exprVar.Name == changedValueName) {
// 								SetValueDirty();
// 								return;
// 							}
// 						}
// 					}));
// 			_autoDestroyConnections.emplace_back(
// 					collection->OnValueCollectionDeleted.connect([this](eves::ValueCollection *collection) {
// 						auto collectionIter = _valueCollections.begin();
// 						while (collectionIter != _valueCollections.end()) {
// 							if (collectionIter->Collection == collection) {
// 								_valueCollections.erase(collectionIter);
// 								return;
// 							}
// 							++collectionIter;
// 						}
// 					}));
// 		}
// 		SetValueDirty();
// 	}
//
// private:
// 	Expression *_expression;
// 	struct SingleValue {
// 		BaseValue *Value;
// 		bool RemoveAfterEval = false;
// 	};
// 	std::unordered_map<std::string, SingleValue> _singleNamedValues;
//
// 	struct ValueCollectionWithPrefix {
// 		std::string Prefix;
// 		ValueCollection *Collection;
// 		bool RemoveAfterEval = false;
// 	};
// 	std::vector<ValueCollectionWithPrefix> _valueCollections;
//
// protected:
// 	void RecalculateCachedValue() override {
// 		if (!_expression)
// 			return;
// 		_expression->ResetVariablesToDefault();
// 		for (auto &exprVar : *_expression) {
// 			auto singleNamedIter = _singleNamedValues.find(exprVar.Name);
// 			if (singleNamedIter != _singleNamedValues.end()) {
// 				exprVar.CurrentValue = singleNamedIter->second.Value->GetValue();
// 				continue;
// 			}
// 			for (auto &valueCollection : _valueCollections) {
// 				if (valueCollection.Prefix.size() >= exprVar.Name.size())
// 					continue;
// 				if (valueCollection.Prefix.empty()) {
// 					auto valueInCollection = valueCollection.Collection->find(exprVar.Name);
// 					if (valueInCollection != valueCollection.Collection->end()) {
// 						exprVar.CurrentValue = valueInCollection->second->GetValue();
// 						break;
// 					}
// 				} else {
// 					// does the variable name start with the prefix?
// 					if (exprVar.Name.rfind(valueCollection.Prefix, 0) != 0)
// 						continue;
// 					std::string varNameWithoutPrefix(exprVar.Name.begin() + valueCollection.Prefix.size(), exprVar.Name.end());
// 					auto valueInCollection = valueCollection.Collection->find(varNameWithoutPrefix);
// 					if (valueInCollection != valueCollection.Collection->end()) {
// 						exprVar.CurrentValue = valueInCollection->second->GetValue();
// 						break;
// 					}
// 				}
// 			}
// 		}
// 		// variables were set. expression can be evaluated now.
// 		double eval = _expression->Evaluate();
// 		CacheValue(static_cast<float>(eval));
//
// 		// all temporary values can be removed now.
// 		bool setDirty = false;
// 		auto singleValueIter = _singleNamedValues.begin();
// 		while (singleValueIter != _singleNamedValues.end()) {
// 			if (singleValueIter->second.RemoveAfterEval) {
// 				singleValueIter = _singleNamedValues.erase(singleValueIter);
// 				setDirty = true;
// 			} else
// 				++singleValueIter;
// 		}
// 		auto collectionIter = _valueCollections.begin();
// 		while (collectionIter != _valueCollections.end()) {
// 			if (collectionIter->RemoveAfterEval) {
// 				collectionIter = _valueCollections.erase(collectionIter);
// 				setDirty = true;
// 			} else
// 				++collectionIter;
// 		}
// 		if (setDirty)
// 			SetValueDirty();
// 	}
//
// private:
// 	std::vector<fteng::connection> _autoDestroyConnections;
// };

} //namespace eves

#endif //EVES_EXPRESSIONVALUE_H
