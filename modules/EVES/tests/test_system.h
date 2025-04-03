#ifndef TEST_SYSTEM_H
#define TEST_SYSTEM_H

#include "tests/test_macros.h"

#include "modules/EVES/System.h"

TEST_CASE("[eves] EntityFlatValueContainer and EntitySumValueContainer") {
    eves::System system;
    eves::EntityFlatValueContainer* flatValueContainer = system.CreateEntityFlatValueContainer("testValue1");
    CHECK(flatValueContainer != nullptr);

    eves::Entity entity(1);
    system.AddEntityToSystem(&entity);
    flatValueContainer->SetValueForEntity(entity.GetID(), 11);
    CHECK(flatValueContainer->GetValueForEntity(entity.GetID()) == 11);

    float value = -1;
    CHECK(entity.GetValueCollection()->GetValue("testValue1", value));
    CHECK(value == 11);

    flatValueContainer->IncrementValueForEntity(entity.GetID(), 100);
    CHECK(flatValueContainer->GetValueForEntity(entity.GetID()) == 111);
    flatValueContainer->SetValueForEntity(entity.GetID(), 1);
    CHECK(flatValueContainer->GetValueForEntity(entity.GetID()) == 1);

    Ref<eves::FlatValueResource> valueRes = flatValueContainer->GetValueResourceForEntity(entity.GetID());
    CHECK(valueRes.is_valid());
    CHECK(valueRes->IsValueValid());
    CHECK(valueRes->GetValue() == 1);

    system.RemoveEntityFromSystem(&entity);
    CHECK(!entity.GetValueCollection()->HasValue("testValue1"));
    CHECK(!valueRes->IsValueValid());

    // having an entity in the system (and therefore values in the value stores), should not
    // trigger an error when the system gets deleted.
    system.AddEntityToSystem(&entity);
}


#endif //TEST_SYSTEM_H
