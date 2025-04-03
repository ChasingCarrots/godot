#include "LocatorSystem.h"
#include "GameObject.h"
#include <core/profiling.h>
#include <core/math/geometry_2d.h>
#include <algorithm>
#include <map>

const float GridSize = 50.0f;

inline Vector2i posToCell(Vector2 pos) {
    return {
            (int) (pos.x / GridSize),
            (int) (pos.y / GridSize)
    };
}


struct LocatorPool {
    String PoolName;
    OAHashMap<Vector2i, Vector<Locator*>> Cells;
};
Vector<LocatorPool> GlobalLocatorPools;
Vector<Vector2i> TempCellCoords;


LocatorSystem::LocatorSystem() {

}

int LocatorSystem::GetTotalNumberOfCells() {
	int numberOfCells = 0;
	for(const auto &pool : GlobalLocatorPools) {
		numberOfCells += pool.Cells.get_num_elements();
	}
	return numberOfCells;
}

int LocatorSystem::GetTotalNumberOfLocators() {
	int numberOfLocators = 0;
	for(const auto &pool : GlobalLocatorPools) {
		auto iter = pool.Cells.iter();
		while(iter.valid) {
			numberOfLocators += iter.value->size();
			iter = pool.Cells.next_iter(iter);
		}
	}
	return numberOfLocators;
}



void LocatorSystem::_bind_methods()
{
    // Bind a method to this class.
    ClassDB::bind_method(D_METHOD("update"), &LocatorSystem::Update);
    ClassDB::bind_method(D_METHOD("get_locators_in_circle", "poolName", "center", "radius", "sort_by_distance"), &LocatorSystem::GetLocatorsInCircle, DEFVAL(false));
    ClassDB::bind_method(D_METHOD("count_locators_in_circle", "poolName", "center", "radius"), &LocatorSystem::CountLocatorsInCircle);
    ClassDB::bind_method(D_METHOD("get_gameobjects_in_circle", "poolName", "center", "radius"), &LocatorSystem::GetGameObjectsInCircle);
    ClassDB::bind_method(D_METHOD("get_locators_outside_circle", "poolName", "center", "radius"), &LocatorSystem::GetLocatorsOutsideCircle);
    ClassDB::bind_method(D_METHOD("count_locators_outside_circle", "poolName", "center", "radius"), &LocatorSystem::CountLocatorsOutsideCircle);
    ClassDB::bind_method(D_METHOD("get_gameobjects_outside_circle", "poolName", "center", "radius"), &LocatorSystem::GetGameObjectsOutsideCircle);
    ClassDB::bind_method(D_METHOD("get_locators_in_circle_motion", "poolName", "center", "radius", "motion"), &LocatorSystem::GetLocatorsInCircleMotion);
    ClassDB::bind_method(D_METHOD("count_locators_in_circle_motion", "poolName", "center", "radius", "motion"), &LocatorSystem::CountLocatorsInCircleMotion);
    ClassDB::bind_method(D_METHOD("get_gameobjects_in_circle_motion", "poolName", "center", "radius", "motion"), &LocatorSystem::GetGameObjectsInCircleMotion);
    ClassDB::bind_method(D_METHOD("get_locators_outside_rectangle", "poolName", "minX", "maxX", "minY", "maxY"), &LocatorSystem::GetLocatorsOutsideRectangle);
    ClassDB::bind_method(D_METHOD("count_locators_outside_rectangle", "poolName", "minX", "maxX", "minY", "maxY"), &LocatorSystem::CountLocatorsOutsideRectangle);
    ClassDB::bind_method(D_METHOD("get_gameobjects_outside_rectangle", "poolName", "minX", "maxX", "minY", "maxY"), &LocatorSystem::GetGameObjectsOutsideRectangle);
    ClassDB::bind_method(D_METHOD("get_locators_in_rectangle", "poolName", "minX", "maxX", "minY", "maxY"), &LocatorSystem::GetLocatorsInRectangle);
    ClassDB::bind_method(D_METHOD("count_locators_in_rectangle", "poolName", "minX", "maxX", "minY", "maxY"), &LocatorSystem::CountLocatorsInRectangle);
    ClassDB::bind_method(D_METHOD("get_gameobjects_in_rectangle", "poolName", "minX", "maxX", "minY", "maxY"), &LocatorSystem::GetGameObjectsInRectangle);
    ClassDB::bind_method(D_METHOD("get_random_locator_in_pool", "poolName"), &LocatorSystem::GetRandomLocatorInPool);
	ClassDB::bind_method(D_METHOD("get_all_locators_in_pool", "poolName"), &LocatorSystem::GetAllLocatorsInPool);
	ClassDB::bind_method(D_METHOD("get_all_gameobjects_in_pool", "poolName"), &LocatorSystem::GetAllGameObjectsInPool);

	ClassDB::bind_static_method("LocatorSystem", D_METHOD("GetTotalNumberOfCells"), &LocatorSystem::GetTotalNumberOfCells);
	ClassDB::bind_static_method("LocatorSystem", D_METHOD("GetTotalNumberOfLocators"), &LocatorSystem::GetTotalNumberOfLocators);
}

void LocatorSystem::LocatorEnteredTree(Locator *locator) {
	PROFILE_FUNCTION()
	auto poolIter = GlobalLocatorPools.begin();
	while (poolIter != GlobalLocatorPools.end()) {
		if (poolIter->PoolName == locator->GetLocatorPoolName())
			break;
		++poolIter;
	}
	if (poolIter == GlobalLocatorPools.end()) {
		GlobalLocatorPools.push_back({ locator->GetLocatorPoolName() });
		poolIter = --GlobalLocatorPools.end();
	}
	auto cell = posToCell(locator->get_global_position());
	locator->SetCurrentCell(cell);
	auto cellVectorPtr = poolIter->Cells.lookup_ptr(cell);
	if (cellVectorPtr == nullptr) {
		poolIter->Cells.insert(cell, { locator });
	} else {
		if (cellVectorPtr->find(locator) == -1)
			cellVectorPtr->push_back(locator);
	}
}

void remove_at_unordered(Vector<Locator *> *locator_vector_ptr, Vector<Locator *>::Size index) {
	if (index < locator_vector_ptr->size() - 1) {
		// godot Vector doesn't have remove_at_unordered, have to do it ourselves
		locator_vector_ptr->set(index, locator_vector_ptr->get(locator_vector_ptr->size() - 1));
		locator_vector_ptr->remove_at(locator_vector_ptr->size() - 1);
	} else {
		// when the locator is already the last one, we can use remove_at...
		locator_vector_ptr->remove_at(index);
	}
}
void LocatorSystem::LocatorExitedTree(Locator *locator) {
	PROFILE_FUNCTION()
    for(auto& pool : GlobalLocatorPools) {
        if(pool.PoolName != locator->GetLocatorPoolName())
            continue; // not the right pool
		auto locatorCellCoord = locator->GetCurrentCell();
		auto cellVectorPtr = pool.Cells.lookup_ptr(locatorCellCoord);
        if(cellVectorPtr == nullptr)
            return; // our cell isn't even there anymore
        auto locatorIndex = cellVectorPtr->find(locator);
        if(locatorIndex == -1)
            return; // our locator wasn't found in the cell

    	remove_at_unordered(cellVectorPtr, locatorIndex);
        if(cellVectorPtr->is_empty())
            // when there are no locators in this cell anymore, delete the cell
            pool.Cells.remove(locatorCellCoord);
        return;
    }
}

void LocatorSystem::Update() {
	PROFILE_FUNCTION()
    for(auto& pool : GlobalLocatorPools) {
        _tempLocators.clear();
    	TempCellCoords.clear();
        auto cellIter = pool.Cells.iter();
        while(cellIter.valid) {
        	for (int locatorIndex=0; locatorIndex < cellIter.value->size();) {
        		Locator* locatorIter = cellIter.value->get(locatorIndex);
        		auto currentLocatorCell = posToCell(locatorIter->get_global_position());
        		if(currentLocatorCell != *cellIter.key){
        			locatorIter->SetCurrentCell(currentLocatorCell);
        			_tempLocators.push_back(locatorIter);
        			remove_at_unordered(cellIter.value, locatorIndex);
        			// locator iterator and index stay the same!
        		}
        		else {
        			++locatorIndex;
        		}
        	}
            if(cellIter.value->is_empty())
                TempCellCoords.append(*cellIter.key);
            cellIter = pool.Cells.next_iter(cellIter);
        }
    	for (auto removeCellCoord : TempCellCoords) {
    		pool.Cells.remove(removeCellCoord);
    	}
        for(auto relocator : _tempLocators) {
            auto cellVectorPtr = pool.Cells.lookup_ptr(relocator->GetCurrentCell());
            if(cellVectorPtr == nullptr) {
                pool.Cells.insert(relocator->GetCurrentCell(), {relocator});
            }
            else {
                if(cellVectorPtr->find(relocator) == -1)
                    cellVectorPtr->push_back(relocator);
            }
        }
    }
}

Array LocatorSystem::GetLocatorsInCircle(String poolName, Vector2 center, float radius, bool sort_by_dist) {
	PROFILE_FUNCTION()
    auto poolIter = GlobalLocatorPools.begin();
    while(poolIter != GlobalLocatorPools.end()) {
        if(poolIter->PoolName == poolName)
            break;
        ++poolIter;
    }
    if(poolIter == GlobalLocatorPools.end())
        return {};

    int minX = int(floor((center.x - radius)/GridSize));
    int minY = int(floor((center.y - radius)/GridSize));
    int maxX = int(ceil((center.x + radius)/GridSize));
    int maxY = int(ceil((center.y + radius)/GridSize));

    Array fillArray;
	LocalVector<float> distSortVector;

    for(int x=minX; x <= maxX; x++) {
        for(int y=minY; y <= maxY; y++) {
            Vector2i cell(x,y);
            auto cellVectorPtr = poolIter->Cells.lookup_ptr(cell);
            if(cellVectorPtr == nullptr)
                continue;
            for(const auto locator : *cellVectorPtr) {
                if(locator->is_queued_for_deletion())
                    continue;
                float checkDistSQ = radius + locator->GetRadius();
                checkDistSQ *= checkDistSQ;
                if(center.distance_squared_to(locator->get_global_position()) <= checkDistSQ) {
                	if(!sort_by_dist)
						fillArray.append(locator);
                	else {
                		int sortIndex = 0;
						for (; sortIndex < distSortVector.size(); ++sortIndex) {
							if(distSortVector[sortIndex] > checkDistSQ)
								break;
						}
                		distSortVector.insert(sortIndex, checkDistSQ);
                		fillArray.insert(sortIndex, locator);
                	}
                }
            }
        }
    }
    return fillArray;
}

int LocatorSystem::CountLocatorsInCircle(String poolName, Vector2 center, float radius) {
	PROFILE_FUNCTION()
    auto poolIter = GlobalLocatorPools.begin();
    while(poolIter != GlobalLocatorPools.end()) {
        if(poolIter->PoolName == poolName)
            break;
        ++poolIter;
    }
    if(poolIter == GlobalLocatorPools.end())
        return 0;

    int minX = int(floor((center.x - radius)/GridSize));
    int minY = int(floor((center.y - radius)/GridSize));
    int maxX = int(ceil((center.x + radius)/GridSize));
    int maxY = int(ceil((center.y + radius)/GridSize));

    int numLocators = 0;

    for(int x=minX; x <= maxX; x++) {
        for(int y=minY; y <= maxY; y++) {
            Vector2i cell(x,y);
            auto cellVectorPtr = poolIter->Cells.lookup_ptr(cell);
            if(cellVectorPtr == nullptr)
                continue;
            for(const auto locator : *cellVectorPtr) {
                if(locator->is_queued_for_deletion())
                    continue;
                float checkDistSQ = radius + locator->GetRadius();
                checkDistSQ *= checkDistSQ;
                if(center.distance_squared_to(locator->get_global_position()) <= checkDistSQ)
                    ++numLocators;
            }
        }
    }
    return numLocators;
}

Array LocatorSystem::GetGameObjectsInCircle(String poolName, Vector2 center, float radius) {
	PROFILE_FUNCTION()
    auto tempArray = GetLocatorsInCircle(poolName, center, radius);
    for(int i=tempArray.size()-1; i >= 0; --i) {
        Node* gameObject = GameObject::getGameObjectInParents(Object::cast_to<Node>(tempArray[i]));
        if(gameObject == nullptr)
            tempArray.remove_at(i);
        else
            tempArray[i] = gameObject;
    }
    return tempArray;
}

Array LocatorSystem::GetLocatorsOutsideCircle(String poolName, Vector2 center, float radius) {
	PROFILE_FUNCTION()
    auto poolIter = GlobalLocatorPools.begin();
    while(poolIter != GlobalLocatorPools.end()) {
        if(poolIter->PoolName == poolName)
            break;
        ++poolIter;
    }
    if(poolIter == GlobalLocatorPools.end())
        return {};

    int minX = int(floor((center.x - radius)/GridSize));
    int minY = int(floor((center.y - radius)/GridSize));
    int maxX = int(ceil((center.x + radius)/GridSize));
    int maxY = int(ceil((center.y + radius)/GridSize));

    Array fillArray;

	auto cellIter = poolIter->Cells.iter();
    while (cellIter.valid) {
        auto cellCoord = *cellIter.key;
        if(cellCoord.x > minX && cellCoord.x < maxX && cellCoord.y > minY && cellCoord.y < maxY) {
        	cellIter = poolIter->Cells.next_iter(cellIter);
	        continue;
        }
        for(auto locator : *cellIter.value) {
            if(locator->is_queued_for_deletion())
                continue;
            float checkDistSQ = radius + locator->GetRadius();
            checkDistSQ *= checkDistSQ;
            if(center.distance_squared_to(locator->get_global_position()) > checkDistSQ)
                fillArray.append(locator);
        }
    	cellIter = poolIter->Cells.next_iter(cellIter);
    }
    return fillArray;
}

int LocatorSystem::CountLocatorsOutsideCircle(String poolName, Vector2 center, float radius) {
	PROFILE_FUNCTION()
    auto poolIter = GlobalLocatorPools.begin();
    while(poolIter != GlobalLocatorPools.end()) {
        if(poolIter->PoolName == poolName)
            break;
        ++poolIter;
    }
    if(poolIter == GlobalLocatorPools.end())
        return 0;

    int minX = int(floor((center.x - radius)/GridSize));
    int minY = int(floor((center.y - radius)/GridSize));
    int maxX = int(ceil((center.x + radius)/GridSize));
    int maxY = int(ceil((center.y + radius)/GridSize));

    int numLocators = 0;

	auto cellIter = poolIter->Cells.iter();
	while (cellIter.valid) {
		auto cellCoord = *cellIter.key;
		if(cellCoord.x > minX && cellCoord.x < maxX && cellCoord.y > minY && cellCoord.y < maxY) {
			cellIter = poolIter->Cells.next_iter(cellIter);
			continue;
		}
        for(auto locator : *cellIter.value) {
            if(locator->is_queued_for_deletion())
                continue;
            float checkDistSQ = radius + locator->GetRadius();
            checkDistSQ *= checkDistSQ;
            if(center.distance_squared_to(locator->get_global_position()) > checkDistSQ)
                ++numLocators;
        }
		cellIter = poolIter->Cells.next_iter(cellIter);
    }
    return numLocators;
}

Array LocatorSystem::GetGameObjectsOutsideCircle(String poolName, Vector2 center, float radius) {
	PROFILE_FUNCTION()
    auto tempArray = GetLocatorsOutsideCircle(poolName, center, radius);
    for(int i=tempArray.size()-1; i >= 0; --i) {
        Node* gameObject = GameObject::getGameObjectInParents(Object::cast_to<Node>(tempArray[i]));
        if(gameObject == nullptr)
            tempArray.remove_at(i);
        else
            tempArray[i] = gameObject;
    }
    return tempArray;
}

Array LocatorSystem::GetLocatorsInCircleMotion(String poolName, Vector2 center, float radius,
                                              Vector2 motion) {
	PROFILE_FUNCTION()
    auto poolIter = GlobalLocatorPools.begin();
    while(poolIter != GlobalLocatorPools.end()) {
        if(poolIter->PoolName == poolName)
            break;
        ++poolIter;
    }
    if(poolIter == GlobalLocatorPools.end())
        return {};

    int minX = int(floor((center.x - radius)/GridSize));
    minX = std::min(minX, int(floor((center.x + motion.x - radius)/GridSize)));
    int minY = int(floor((center.y - radius)/GridSize));
    minY = std::min(minY, int(floor((center.y + motion.y - radius)/GridSize)));
    int maxX = int(ceil((center.x + radius)/GridSize));
    maxX = std::max(maxX, int(ceil((center.x + motion.x + radius)/GridSize)));
    int maxY = int(ceil((center.y + radius)/GridSize));
    maxY = std::max(maxY, int(ceil((center.y + motion.y + radius)/GridSize)));

    Array fillArray;

    for(int x=minX; x <= maxX; x++) {
        for(int y=minY; y <= maxY; y++) {
            Vector2i cell(x,y);
            auto cellVectorPtr = poolIter->Cells.lookup_ptr(cell);
            if(cellVectorPtr == nullptr)
                continue;
            for(const auto locator : *cellVectorPtr) {
                if(locator->is_queued_for_deletion())
                    continue;
				Vector2 segment[2] = {center, center+motion};
                Vector2 checkPoint = Geometry2D::get_closest_point_to_segment(
                        locator->get_global_position(), segment);
                float checkDistSQ = radius + locator->GetRadius();
                checkDistSQ *= checkDistSQ;
                if(checkPoint.distance_squared_to(locator->get_global_position()) <= checkDistSQ)
                    fillArray.append(locator);
            }
        }
    }
    return fillArray;
}

int LocatorSystem::CountLocatorsInCircleMotion(String poolName, Vector2 center, float radius,
                                                      Vector2 motion) {
	PROFILE_FUNCTION()
    auto poolIter = GlobalLocatorPools.begin();
    while(poolIter != GlobalLocatorPools.end()) {
        if(poolIter->PoolName == poolName)
            break;
        ++poolIter;
    }
    if(poolIter == GlobalLocatorPools.end())
        return 0;

    int minX = int(floor((center.x - radius)/GridSize));
    minX = std::min(minX, int(floor((center.x + motion.x - radius)/GridSize)));
    int minY = int(floor((center.y - radius)/GridSize));
    minY = std::min(minY, int(floor((center.y + motion.y - radius)/GridSize)));
    int maxX = int(ceil((center.x + radius)/GridSize));
    maxX = std::max(maxX, int(ceil((center.x + motion.x + radius)/GridSize)));
    int maxY = int(ceil((center.y + radius)/GridSize));
    maxY = std::max(maxY, int(ceil((center.y + motion.y + radius)/GridSize)));

    int numLocators = 0;

    for(int x=minX; x <= maxX; x++) {
        for(int y=minY; y <= maxY; y++) {
            Vector2i cell(x,y);
            auto cellVectorPtr = poolIter->Cells.lookup_ptr(cell);
            if(cellVectorPtr == nullptr)
                continue;
            for(const auto locator : *cellVectorPtr) {
                if(locator->is_queued_for_deletion())
                    continue;
				Vector2 segment[2] = {center, center+motion};
				Vector2 checkPoint = Geometry2D::get_closest_point_to_segment(
						locator->get_global_position(), segment);
                float checkDistSQ = radius + locator->GetRadius();
                checkDistSQ *= checkDistSQ;
                if(checkPoint.distance_squared_to(locator->get_global_position()) <= checkDistSQ)
                    ++numLocators;
            }
        }
    }
    return numLocators;
}

Array LocatorSystem::GetGameObjectsInCircleMotion(String poolName, Vector2 center, float radius,
                                                 Vector2 motion) {
	PROFILE_FUNCTION()
    auto tempArray = GetLocatorsInCircleMotion(poolName, center, radius, motion);
    for(int i=tempArray.size()-1; i >= 0; --i) {
        Node* gameObject = GameObject::getGameObjectInParents(Object::cast_to<Node>(tempArray[i]));
        if(gameObject == nullptr)
            tempArray.remove_at(i);
        else
            tempArray[i] = gameObject;
    }
    return tempArray;
}

Array LocatorSystem::GetLocatorsOutsideRectangle(String poolName, float minX, float maxX, float minY, float maxY) {
	PROFILE_FUNCTION()
    auto poolIter = GlobalLocatorPools.begin();
    while(poolIter != GlobalLocatorPools.end()) {
        if(poolIter->PoolName == poolName)
            break;
        ++poolIter;
    }
    if(poolIter == GlobalLocatorPools.end())
        return {};

    int minXCoord = int(floor(minX / GridSize));
    int maxXCoord = int(ceil(minX / GridSize));
    int minYCoord = int(floor(minY / GridSize));
    int maxYCoord = int(ceil(minY / GridSize));

    Array fillArray;

	auto cellIter = poolIter->Cells.iter();
	while (cellIter.valid) {
		auto cellCoord = *cellIter.key;
		if(cellCoord.x > minXCoord && cellCoord.x < maxXCoord && cellCoord.y > minYCoord && cellCoord.y < maxYCoord) {
			cellIter = poolIter->Cells.next_iter(cellIter);
			continue;
		}
        for(auto locator : *cellIter.value) {
            if(locator->is_queued_for_deletion())
                continue;
            auto locPos = locator->get_global_position();
            if(locPos.x < minX || locPos.x > maxX || locPos.y < minY || locPos.y > maxY)
                fillArray.append(locator);
        }
		cellIter = poolIter->Cells.next_iter(cellIter);
    }
    return fillArray;
}

int LocatorSystem::CountLocatorsOutsideRectangle(String poolName, float minX, float maxX, float minY, float maxY) {
	PROFILE_FUNCTION()
    auto poolIter = GlobalLocatorPools.begin();
    while(poolIter != GlobalLocatorPools.end()) {
        if(poolIter->PoolName == poolName)
            break;
        ++poolIter;
    }
    if(poolIter == GlobalLocatorPools.end())
        return 0;

    int minXCoord = int(floor(minX / GridSize));
    int maxXCoord = int(ceil(minX / GridSize));
    int minYCoord = int(floor(minY / GridSize));
    int maxYCoord = int(ceil(minY / GridSize));

    int numLocators = 0;

	auto cellIter = poolIter->Cells.iter();
	while (cellIter.valid) {
		auto cellCoord = *cellIter.key;
		if(cellCoord.x > minXCoord && cellCoord.x < maxXCoord && cellCoord.y > minYCoord && cellCoord.y < maxYCoord) {
			cellIter = poolIter->Cells.next_iter(cellIter);
			continue;
		}
        for(auto locator : *cellIter.value) {
            if(locator->is_queued_for_deletion())
                continue;
            auto locPos = locator->get_global_position();
            if(locPos.x < minX || locPos.x > maxX || locPos.y < minY || locPos.y > maxY)
                ++numLocators;
        }
		cellIter = poolIter->Cells.next_iter(cellIter);
    }
    return numLocators;
}

Array LocatorSystem::GetGameObjectsOutsideRectangle(String poolName, float minX, float maxX, float minY, float maxY) {
	PROFILE_FUNCTION()
    auto tempArray = GetLocatorsOutsideRectangle(poolName, minX, maxX, minY, maxY);
    for(int i=tempArray.size()-1; i >= 0; --i) {
        Node* gameObject = GameObject::getGameObjectInParents(Object::cast_to<Node>(tempArray[i]));
        if(gameObject == nullptr)
            tempArray.remove_at(i);
        else
            tempArray[i] = gameObject;
    }
    return tempArray;
}

Array LocatorSystem::GetLocatorsInRectangle(String poolName, float minX, float maxX, float minY, float maxY) {
	PROFILE_FUNCTION()
    auto poolIter = GlobalLocatorPools.begin();
    while(poolIter != GlobalLocatorPools.end()) {
        if(poolIter->PoolName == poolName)
            break;
        ++poolIter;
    }
    if(poolIter == GlobalLocatorPools.end())
        return {};

    int minXCoord = int(floor(minX / GridSize));
    int maxXCoord = int(ceil(minX / GridSize));
    int minYCoord = int(floor(minY / GridSize));
    int maxYCoord = int(ceil(minY / GridSize));

    Array fillArray;

    for(int x=minXCoord; x <= maxXCoord; x++) {
        for(int y=minYCoord; y <= maxYCoord; y++) {
            Vector2i cell(x,y);
            auto cellVectorPtr = poolIter->Cells.lookup_ptr(cell);
            if(cellVectorPtr == nullptr)
                continue;
            for(const auto locator : *cellVectorPtr) {
                if(locator->is_queued_for_deletion())
                    continue;
                auto locPos = locator->get_global_position();
                if(locPos.x > minX && locPos.x < maxX && locPos.y > minY && locPos.y < maxY)
                    fillArray.append(locator);
            }
        }
    }
    return fillArray;
}

int LocatorSystem::CountLocatorsInRectangle(String poolName, float minX, float maxX, float minY, float maxY) {
	PROFILE_FUNCTION()
    auto poolIter = GlobalLocatorPools.begin();
    while(poolIter != GlobalLocatorPools.end()) {
        if(poolIter->PoolName == poolName)
            break;
        ++poolIter;
    }
    if(poolIter == GlobalLocatorPools.end())
        return 0;

    int minXCoord = int(floor(minX / GridSize));
    int maxXCoord = int(ceil(minX / GridSize));
    int minYCoord = int(floor(minY / GridSize));
    int maxYCoord = int(ceil(minY / GridSize));

    int numLocators = 0;

    for(int x=minXCoord; x <= maxXCoord; x++) {
        for(int y=minYCoord; y <= maxYCoord; y++) {
            Vector2i cell(x,y);
            auto cellIter = poolIter->Cells.lookup_ptr(cell);
            if(cellIter == nullptr)
                continue;
            for(const auto locator : *cellIter) {
                if(locator->is_queued_for_deletion())
                    continue;
                auto locPos = locator->get_global_position();
                if(locPos.x > minX && locPos.x < maxX && locPos.y > minY && locPos.y < maxY)
                    ++numLocators;
            }
        }
    }
    return numLocators;
}

Array LocatorSystem::GetGameObjectsInRectangle(String poolName, float minX, float maxX, float minY, float maxY) {
	PROFILE_FUNCTION()
    auto tempArray = GetLocatorsInRectangle(poolName, minX, maxX, minY, maxY);
    for(int i=tempArray.size()-1; i >= 0; --i) {
        Node* gameObject = GameObject::getGameObjectInParents(Object::cast_to<Node>(tempArray[i]));
        if(gameObject == nullptr)
            tempArray.remove_at(i);
        else
            tempArray[i] = gameObject;
    }
    return tempArray;
}

Locator* LocatorSystem::GetRandomLocatorInPool(String poolName) {
	PROFILE_FUNCTION()
    auto poolIter = GlobalLocatorPools.begin();
    while(poolIter != GlobalLocatorPools.end()) {
        if(poolIter->PoolName == poolName)
            break;
        ++poolIter;
    }
    if(poolIter == GlobalLocatorPools.end())
        return nullptr;

    _tempLocators.clear();
	auto cellIter = poolIter->Cells.iter();
	while (cellIter.valid) {
        for(auto locator : *cellIter.value)
            _tempLocators.push_back(locator);
		cellIter = poolIter->Cells.next_iter(cellIter);
    }
    if(_tempLocators.empty())
        return nullptr;
    int index = _random.randi_range(0, _tempLocators.size()-1);
    return _tempLocators[index];
}

Array LocatorSystem::GetAllLocatorsInPool(String poolName) {
	PROFILE_FUNCTION()
	Array fillArray;

	auto poolIter = GlobalLocatorPools.begin();
	while(poolIter != GlobalLocatorPools.end()) {
		if(poolIter->PoolName == poolName)
			break;
		++poolIter;
	}
	if(poolIter == GlobalLocatorPools.end())
		return {};

	_tempLocators.clear();
	auto cellIter = poolIter->Cells.iter();
	while (cellIter.valid) {
		for(auto locator : *cellIter.value)
			fillArray.append(locator);
		cellIter = poolIter->Cells.next_iter(cellIter);
	}

	return fillArray;
}

Array LocatorSystem::GetAllGameObjectsInPool(String poolName) {
	PROFILE_FUNCTION()
	Array fillArray;

	auto poolIter = GlobalLocatorPools.begin();
	while (poolIter != GlobalLocatorPools.end()) {
		if (poolIter->PoolName == poolName)
			break;
		++poolIter;
	}
	if (poolIter == GlobalLocatorPools.end())
		return {};

	_tempLocators.clear();
	auto cellIter = poolIter->Cells.iter();
	while (cellIter.valid) {
		for (auto locator : *cellIter.value) {
			Node *gameObject = GameObject::getGameObjectInParents(Object::cast_to<Node>(locator));
			if (gameObject != nullptr)
				fillArray.append(gameObject);
		}
		cellIter = poolIter->Cells.next_iter(cellIter);
	}

	return fillArray;
}

void LocatorSystem::FillWithGameObjectsInCircleMotion(String poolName, Vector2 center, float radius, Vector2 motion, LocalVector<GameObject *> &fillVector) {
	PROFILE_FUNCTION()
	auto poolIter = GlobalLocatorPools.begin();
	while(poolIter != GlobalLocatorPools.end()) {
		if(poolIter->PoolName == poolName)
			break;
		++poolIter;
	}
	if(poolIter == GlobalLocatorPools.end())
		return;

	int minX = int(floor((center.x - radius)/GridSize));
	minX = std::min(minX, int(floor((center.x + motion.x - radius)/GridSize)));
	int minY = int(floor((center.y - radius)/GridSize));
	minY = std::min(minY, int(floor((center.y + motion.y - radius)/GridSize)));
	int maxX = int(ceil((center.x + radius)/GridSize));
	maxX = std::max(maxX, int(ceil((center.x + motion.x + radius)/GridSize)));
	int maxY = int(ceil((center.y + radius)/GridSize));
	maxY = std::max(maxY, int(ceil((center.y + motion.y + radius)/GridSize)));

	for(int x=minX; x <= maxX; x++) {
		for(int y=minY; y <= maxY; y++) {
			Vector2i cell(x,y);
			auto cellVectorPtr = poolIter->Cells.lookup_ptr(cell);
			if(cellVectorPtr == nullptr)
				continue;
			for(const auto locator : *cellVectorPtr) {
				if(locator->is_queued_for_deletion())
					continue;
				Vector2 segment[2] = {center, center+motion};
				Vector2 checkPoint = Geometry2D::get_closest_point_to_segment(
						locator->get_global_position(), segment);
				float checkDistSQ = radius + locator->GetRadius();
				checkDistSQ *= checkDistSQ;
				if(checkPoint.distance_squared_to(locator->get_global_position()) <= checkDistSQ) {
					GameObject* gameObject = GameObject::getGameObjectInParents(locator);
					if(gameObject != nullptr)
						fillVector.push_back(gameObject);
				}
			}
		}
	}
}

void LocatorSystem::FillWithGameObjectsInCircle(String poolName, Vector2 center, float radius, LocalVector<GameObject *> &fillVector) {
	PROFILE_FUNCTION()
	auto poolIter = GlobalLocatorPools.begin();
	while(poolIter != GlobalLocatorPools.end()) {
		if(poolIter->PoolName == poolName)
			break;
		++poolIter;
	}
	if(poolIter == GlobalLocatorPools.end())
		return;

	int minX = int(floor((center.x - radius)/GridSize));
	int minY = int(floor((center.y - radius)/GridSize));
	int maxX = int(ceil((center.x + radius)/GridSize));
	int maxY = int(ceil((center.y + radius)/GridSize));

	for(int x=minX; x <= maxX; x++) {
		for(int y=minY; y <= maxY; y++) {
			Vector2i cell(x,y);
			auto cellVectorPtr = poolIter->Cells.lookup_ptr(cell);
			if(cellVectorPtr == nullptr)
				continue;
			for(const auto locator : *cellVectorPtr) {
				if(locator->is_queued_for_deletion())
					continue;
				float checkDistSQ = radius + locator->GetRadius();
				checkDistSQ *= checkDistSQ;
				if(center.distance_squared_to(locator->get_global_position()) <= checkDistSQ) {
					GameObject* gameObject = GameObject::getGameObjectInParents(locator);
					if(gameObject != nullptr)
						fillVector.push_back(gameObject);
				}
			}
		}
	}
}