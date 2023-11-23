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


namespace std {
    template<>
    struct less<Vector2i> {
        bool operator()(const Vector2i &lhs, const Vector2i &rhs) const {
            return lhs.x < rhs.x || (lhs.x == rhs.x && lhs.y < rhs.y);
        }
    };
}

struct LocatorPool {
    String PoolName;
    std::map<Vector2i, LocalVector<Locator*>> Cells;
};
std::vector<LocatorPool> GlobalLocatorPools;



LocatorSystem::LocatorSystem() {

}


void LocatorSystem::_bind_methods()
{
    // Bind a method to this class.
    ClassDB::bind_method(D_METHOD("update"), &LocatorSystem::Update);
    ClassDB::bind_method(D_METHOD("get_locators_in_circle", "poolName", "center", "radius"), &LocatorSystem::GetLocatorsInCircle);
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
}

void LocatorSystem::LocatorEnteredTree(Locator *locator) {
	PROFILE_FUNCTION()
    auto poolIter = GlobalLocatorPools.begin();
    while(poolIter != GlobalLocatorPools.end()) {
        if(poolIter->PoolName == locator->GetLocatorPoolName())
            break;
        ++poolIter;
    }
    if(poolIter == GlobalLocatorPools.end()) {
        GlobalLocatorPools.push_back({
            locator->GetLocatorPoolName()
        });
        poolIter = --GlobalLocatorPools.end();
    }
    auto cell = posToCell(locator->get_global_position());
    locator->SetCurrentCell(cell);
    auto cellIter = poolIter->Cells.find(cell);
    if(cellIter == poolIter->Cells.end()) {
        poolIter->Cells.insert({cell, {locator}});
    }
    else {
        if(cellIter->second.find(locator) == -1)
            cellIter->second.push_back(locator);
    }
}

void LocatorSystem::LocatorExitedTree(Locator *locator) {
	PROFILE_FUNCTION()
    for(auto& pool : GlobalLocatorPools) {
        if(pool.PoolName != locator->GetLocatorPoolName())
            continue; // not the right pool
        auto cellIter = pool.Cells.find(locator->GetCurrentCell());
        if(cellIter == pool.Cells.end())
            return; // our cell isn't even there anymore
        auto locatorIndex = cellIter->second.find(locator);
        if(locatorIndex == -1)
            return; // our locator wasn't found in the cell
        cellIter->second.remove_at_unordered(locatorIndex);
        if(cellIter->second.is_empty())
            // when there are no locators in this cell anymore, delete the cell
            pool.Cells.erase(cellIter);
        return;
    }
}

void LocatorSystem::Update() {
	PROFILE_FUNCTION()
    for(auto& pool : GlobalLocatorPools) {
        _tempLocators.clear();
        auto cellIter = pool.Cells.begin();
        while(cellIter != pool.Cells.end()) {
			int locatorIndex = 0;
            auto locatorIter = cellIter->second.begin();
            while(locatorIter != cellIter->second.end()) {
                auto currentLocatorCell = posToCell((*locatorIter)->get_global_position());
                if(currentLocatorCell != cellIter->first){
                    (*locatorIter)->SetCurrentCell(currentLocatorCell);
                    _tempLocators.push_back(*locatorIter);
                    cellIter->second.remove_at_unordered(locatorIndex);
					// locator iterator and index stay the same!
                }
                else {
					++locatorIndex;
					++locatorIter;
				}
			}
            if(cellIter->second.is_empty())
                cellIter = pool.Cells.erase(cellIter);
            else
                ++cellIter;
        }
        for(auto relocator : _tempLocators) {
            auto cellIter = pool.Cells.find(relocator->GetCurrentCell());
            if(cellIter == pool.Cells.end()) {
                pool.Cells.insert({relocator->GetCurrentCell(), {relocator}});
            }
            else {
                if(cellIter->second.find(relocator) == -1)
                    cellIter->second.push_back(relocator);
            }
        }
    }
}

Array LocatorSystem::GetLocatorsInCircle(String poolName, Vector2 center, float radius) {
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

    for(int x=minX; x <= maxX; x++) {
        for(int y=minY; y <= maxY; y++) {
            Vector2i cell(x,y);
            auto cellIter = poolIter->Cells.find(cell);
            if(cellIter == poolIter->Cells.end())
                continue;
            for(const auto locator : cellIter->second) {
                if(locator->is_queued_for_deletion())
                    continue;
                float checkDistSQ = radius + locator->GetRadius();
                checkDistSQ *= checkDistSQ;
                if(center.distance_squared_to(locator->get_global_position()) <= checkDistSQ)
                    fillArray.append(locator);
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
            auto cellIter = poolIter->Cells.find(cell);
            if(cellIter == poolIter->Cells.end())
                continue;
            for(const auto locator : cellIter->second) {
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

    for(const auto& cell :poolIter->Cells) {
        auto cellCoord = cell.first;
        if(cellCoord.x > minX && cellCoord.x < maxX && cellCoord.y > minY && cellCoord.y < maxY)
            continue;
        for(auto locator : cell.second) {
                if(locator->is_queued_for_deletion())
                    continue;
                float checkDistSQ = radius + locator->GetRadius();
                checkDistSQ *= checkDistSQ;
                if(center.distance_squared_to(locator->get_global_position()) > checkDistSQ)
                    fillArray.append(locator);
            }
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

    for(const auto& cell :poolIter->Cells) {
        auto cellCoord = cell.first;
        if(cellCoord.x > minX && cellCoord.x < maxX && cellCoord.y > minY && cellCoord.y < maxY)
            continue;
        for(auto locator : cell.second) {
            if(locator->is_queued_for_deletion())
                continue;
            float checkDistSQ = radius + locator->GetRadius();
            checkDistSQ *= checkDistSQ;
            if(center.distance_squared_to(locator->get_global_position()) > checkDistSQ)
                ++numLocators;
        }
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
            auto cellIter = poolIter->Cells.find(cell);
            if(cellIter == poolIter->Cells.end())
                continue;
            for(const auto locator : cellIter->second) {
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
            auto cellIter = poolIter->Cells.find(cell);
            if(cellIter == poolIter->Cells.end())
                continue;
            for(const auto locator : cellIter->second) {
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

    for(const auto& cell :poolIter->Cells) {
        auto cellCoord = cell.first;
        if(cellCoord.x > minXCoord && cellCoord.x < maxXCoord && cellCoord.y > minYCoord && cellCoord.y < maxYCoord)
            continue;
        for(auto locator : cell.second) {
            if(locator->is_queued_for_deletion())
                continue;
            auto locPos = locator->get_global_position();
            if(locPos.x < minX || locPos.x > maxX || locPos.y < minY || locPos.y > maxY)
                fillArray.append(locator);
        }
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

    for(const auto& cell :poolIter->Cells) {
        auto cellCoord = cell.first;
        if(cellCoord.x > minXCoord && cellCoord.x < maxXCoord && cellCoord.y > minYCoord && cellCoord.y < maxYCoord)
            continue;
        for(auto locator : cell.second) {
            if(locator->is_queued_for_deletion())
                continue;
            auto locPos = locator->get_global_position();
            if(locPos.x < minX || locPos.x > maxX || locPos.y < minY || locPos.y > maxY)
                ++numLocators;
        }
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
            auto cellIter = poolIter->Cells.find(cell);
            if(cellIter == poolIter->Cells.end())
                continue;
            for(const auto locator : cellIter->second) {
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
            auto cellIter = poolIter->Cells.find(cell);
            if(cellIter == poolIter->Cells.end())
                continue;
            for(const auto locator : cellIter->second) {
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
    for(const auto& cell : poolIter->Cells) {
        for(auto locator : cell.second)
            _tempLocators.push_back(locator);
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
	for(const auto& cell : poolIter->Cells)
		for(auto locator : cell.second)
			fillArray.append(locator);

	return fillArray;
}

Array LocatorSystem::GetAllGameObjectsInPool(String poolName) {
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
	for(const auto& cell : poolIter->Cells)
		for(auto locator : cell.second) {
			Node* gameObject = GameObject::getGameObjectInParents(Object::cast_to<Node>(locator));
			if(gameObject != nullptr)
				fillArray.append(gameObject);
		}

	return fillArray;
}