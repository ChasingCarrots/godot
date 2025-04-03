#ifndef TEST_VALUECOLLECTION_H
#define TEST_VALUECOLLECTION_H

#include "tests/test_macros.h"

#include "modules/EVES/ExpressionValue.h"
#include "modules/EVES/FlatValue.h"
#include "modules/EVES/SumValue.h"
#include "modules/EVES/ValueCollection.h"

TEST_CASE("[eves] FlatValue (+BaseValue)") {
	eves::FlatValue flatValue;
	flatValue.SetValue(99);
	CHECK(flatValue.GetValue() == 99);

	bool valueUpdatedCalledCorrectly = false;
	flatValue.OnValueChanged.connect([&valueUpdatedCalledCorrectly, &flatValue](eves::BaseValue *baseValue) {
		if (baseValue == &flatValue)
			valueUpdatedCalledCorrectly = true;
	});
	flatValue.SetValue(101);
	CHECK(valueUpdatedCalledCorrectly);

	eves::FlatValue *flatValueAsPointer = new eves::FlatValue();
	bool deleteSignalTriggeredCorrectly = false;
	flatValueAsPointer->OnValueDeleted.connect([&deleteSignalTriggeredCorrectly, flatValueAsPointer](eves::BaseValue *baseValue) {
		if (flatValueAsPointer == baseValue)
			deleteSignalTriggeredCorrectly = true;
	});
	delete flatValueAsPointer;
	CHECK(deleteSignalTriggeredCorrectly);
}

TEST_CASE("[eves] SumValue") {
	eves::SumValue sumValue;
	eves::FlatValue flatVal1;
	flatVal1.SetValue(1);
	eves::FlatValue flatVal2;
	flatVal2.SetValue(10);
	eves::FlatValue *flatVal3 = new eves::FlatValue();
	flatVal3->SetValue(100);

	sumValue.AddValueToSum(&flatVal1);
	CHECK(sumValue.GetValue() == flatVal1.GetValue());
	bool sumUpdateTriggeredCorrectly = false;
	sumValue.OnValueChanged.connect([&sumUpdateTriggeredCorrectly, &sumValue](eves::BaseValue *baseValue) {
		if (baseValue == &sumValue)
			sumUpdateTriggeredCorrectly = true;
	});
	sumValue.AddValueToSum(&flatVal2);
	CHECK(sumUpdateTriggeredCorrectly);
	CHECK(sumValue.GetValue() == 11);
	sumUpdateTriggeredCorrectly = false;
	flatVal2.SetValue(20);
	CHECK(sumUpdateTriggeredCorrectly);
	CHECK(sumValue.GetValue() == 21);
	sumValue.AddValueToSum(flatVal3);
	CHECK(sumValue.GetValue() == 121);
	sumUpdateTriggeredCorrectly = false;
	delete flatVal3;
	CHECK(sumUpdateTriggeredCorrectly);
	CHECK(sumValue.GetValue() == 21);
}

TEST_CASE("[eves] Expression") {
	eves::Expression expression("sqrt(x^2+y^2)", { "x", "y" }, { 2, 0 });
	double eval = expression.Evaluate();
	CHECK(eval == 2);
	// begin() and end() simply point to the variable storage,
	// so that variables can be set this way:
	for (auto &var : expression) {
		if (var.Name == "x") {
			var.CurrentValue = 2;
		} else if (var.Name == "y") {
			var.CurrentValue = 5;
		}
	}
	double calculatedResult = sqrt(2.0 * 2.0 + 5.0 * 5.0);
	eval = expression.Evaluate();
	CHECK(eval == calculatedResult);
	expression.ResetVariablesToDefault();
	eval = expression.Evaluate();
	CHECK(eval == 2);
}

TEST_CASE("[eves] ValueCollection") {
	eves::ValueCollection valueCollection;
	// checking that the get/has functions also work, when the collection is empty
	CHECK(!valueCollection.HasValue("test"));
	float value = -1;
	CHECK(!valueCollection.GetValue("test", value));
	CHECK(value == -1);

	// clearing an empty value collection should just work and do nothing...
	valueCollection.ClearAllValues();

	eves::FlatValue testValue1;
	testValue1.SetValue(1);
	eves::FlatValue testValue2;
	testValue2.SetValue(2);
	eves::FlatValue testValue3;
	testValue3.SetValue(3);

	float testValue1_targetValue = 1;
	int testValue1_updateCalled = 0;
	valueCollection.ConnectToValueChangedSignal("testValue1",
			[&testValue1_updateCalled, &testValue1, &testValue1_targetValue](eves::BaseValue *baseValue) {
				testValue1_updateCalled++;
				CHECK(baseValue == &testValue1);
				CHECK(baseValue->GetValue() == testValue1_targetValue);
			});
	CHECK(testValue1_updateCalled == 0);
	valueCollection.AddNamedValue("testValue1", &testValue1);
	// when adding a value and there is already an update callback registered, that should be triggered!
	CHECK(testValue1_updateCalled == 1);
	testValue1_targetValue = 11;
	testValue1.SetValue(testValue1_targetValue);
	CHECK(testValue1_updateCalled == 2);

	valueCollection.AddNamedValue("testValue2", &testValue2);
	// adding another value should not trigger update callbacks of other values
	CHECK(testValue1_updateCalled == 2);
	int testValue2_updateCalled = 0;
	valueCollection.ConnectToValueChangedSignal("testValue2", [&testValue2_updateCalled](eves::BaseValue *) {
		testValue2_updateCalled++;
	});
	// when registering an update callback for a value that is already registered, that callback should
	// not be triggered initially.
	CHECK(testValue2_updateCalled == 0);
	testValue2.SetValue(22);
	CHECK(testValue2_updateCalled == 1);
	// changing another value should not trigger update callbacks of other values
	CHECK(testValue1_updateCalled == 2);

	// testing auto-removing connections to callbacks:
	int testValue3_updateCalled = 0;
	{
		fteng::connection update_conn = valueCollection.ConnectToValueChangedSignal("testValue3", [&testValue3_updateCalled](eves::BaseValue *) {
			testValue3_updateCalled++;
		});
		valueCollection.AddNamedValue("testValue3", &testValue3);
		CHECK(testValue3_updateCalled == 1);
		// when fteng::connection goes out of scope, it will disconnect the signal.
	}
	testValue3.SetValue(33);
	// the connection should be disconnected and the counter not incremented
	CHECK(testValue3_updateCalled == 1);

	bool gotValue = valueCollection.GetValue("testValue3", value);
	CHECK(gotValue);
	CHECK(value == 33);

	CHECK(valueCollection.HasValue("testValue3"));

	eves::BaseValue* gottenValue = valueCollection.GetBaseValue("testValue2");
	CHECK(gottenValue == &testValue2);
	CHECK(valueCollection.GetBaseValue("nonExistant") == nullptr);

	// testing auto-removing values from collections when they get deleted
	int scopedValue_updateCalled = 0;
	valueCollection.ConnectToValueChangedSignal("scopedValue", [&scopedValue_updateCalled](eves::BaseValue *) {
		scopedValue_updateCalled++;
	});
	{
		eves::FlatValue scopedValue;
		scopedValue.SetValue(99);
		valueCollection.AddNamedValue("scopedValue", &scopedValue);
		// the desctructor of ane BaseValue should automatically remove the value
		// from the valueCollection
	}
	CHECK(!valueCollection.HasValue("scopedValue"));
	CHECK(scopedValue_updateCalled == 1);

	// testing having values in multiple collections and deleting collections with values in them
	{
		eves::ValueCollection scopedValueCollection;
		int testValue1_updateCalled_scopedCollection = 0;
		scopedValueCollection.ConnectToValueChangedSignal("testValue1",
				[&testValue1_updateCalled_scopedCollection, &testValue1, &testValue1_targetValue](eves::BaseValue *baseValue) {
					testValue1_updateCalled_scopedCollection++;
					CHECK(baseValue == &testValue1);
					CHECK(baseValue->GetValue() == testValue1_targetValue);
				});

		scopedValueCollection.AddNamedValue("testValue1", &testValue1);
		scopedValueCollection.AddNamedValue("testValue2", &testValue2);
		scopedValueCollection.AddNamedValue("testValue3", &testValue3);
		// having values in multiple collections should be possible
		CHECK(testValue1_updateCalled_scopedCollection == 1);
		CHECK(testValue1_updateCalled == 2);
		testValue1_targetValue = 111;
		testValue1.SetValue(testValue1_targetValue);
		CHECK(testValue1_updateCalled_scopedCollection == 2);
		CHECK(testValue1_updateCalled == 3);
		CHECK(scopedValueCollection.GetValue("testValue1", value));
		CHECK(value == testValue1_targetValue);
		// after letting the scoped value collection go out of scope, the rest should still work as normal
	}
	testValue1_targetValue = 1111;
	testValue1.SetValue(testValue1_targetValue);
	CHECK(testValue1_updateCalled == 4);
	CHECK(valueCollection.GetValue("testValue1", value));
	CHECK(value == testValue1_targetValue);
}

TEST_CASE("[eves] BaseValue overrides") {
	eves::FlatValue value1;
	value1.SetValue(1);
	int value1_updateCalled = 0;
	value1.OnValueChanged.connect([&value1_updateCalled](eves::BaseValue *) {
		value1_updateCalled++;
	});
	eves::FlatValue value2;
	value2.SetValue(2);
	int value2_updateCalled = 0;
	value2.OnValueChanged.connect([&value2_updateCalled](eves::BaseValue *) {
		value2_updateCalled++;
	});
	eves::FlatValue value3;
	value3.SetValue(3);
	int value3_updateCalled = 0;
	value3.OnValueChanged.connect([&value3_updateCalled](eves::BaseValue *) {
		value3_updateCalled++;
	});

	// override chains should be possible and do what one expects
	value1.AddOverride(&value2);
	CHECK(value1_updateCalled == 1);
	CHECK(value1.GetValue() == 2);
	value2.AddOverride(&value3);
	CHECK(value1_updateCalled == 2);
	CHECK(value1.GetValue() == 3);
	CHECK(value2_updateCalled == 1);
	CHECK(value2.GetValue() == 3);
	value2.SetValue(22);
	CHECK(value1_updateCalled == 2);
	CHECK(value1.GetValue() == 3);
	value3.SetValue(33);
	CHECK(value1_updateCalled == 3);
	CHECK(value1.GetValue() == 33);


	// creating a circular override loop should not be possible (and trigger an error...)
	// uncomment to test, but we don't want an error msg to be part of the test in general...
	// value3.AddOverride(&value1);

	// values that get deleted should simply be removed from any override chain
	{
		eves::FlatValue scopedValue;
		scopedValue.SetValue(5);
		value3.AddOverride(&scopedValue);
		CHECK(value1_updateCalled == 4);
		CHECK(value1.GetValue() == 5);
	}
	CHECK(value1_updateCalled == 5);
	CHECK(value1.GetValue() == 33);

	// additional overrides on the same value should be possible (overriding the first override)
	eves::FlatValue value4;
	value4.SetValue(4);
	value1.AddOverride(&value4);
	CHECK(value1_updateCalled == 6);
	CHECK(value1.GetValue() == 4);
	// removing the value that is not the current active override should not trigger an update!
	value1.RemoveOverride(&value2);
	CHECK(value1_updateCalled == 6);
	CHECK(value1.GetValue() == 4);

	// removing the last override should trigger an update and simply return the value back to the non-override state
	value1.RemoveOverride(&value4);
	CHECK(value1_updateCalled == 7);
	CHECK(value1.GetValue() == 1);

	// changing values that were overrides once should not change the value anymore
	value4.SetValue(44);
	value3.SetValue(333);
	value2.SetValue(222);
	CHECK(value1_updateCalled == 7);
	CHECK(value1.GetValue() == 1);
}

#endif //TEST_VALUECOLLECTION_H
