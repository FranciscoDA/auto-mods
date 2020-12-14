#include "genie/dat/DatFile.h"
#include "genie/dat/TechageEffect.h"
#include "patches.h"
#include "ids.h"
#include <list>
#include <random>
#include <set>
#include <algorithm>
#include <map>

using namespace std;

typedef genie::ResourceUsage<int16_t, int16_t, int16_t> ResourceCost;
typedef genie::ResourceUsage<int16_t, int16_t, int8_t> ResearchResourceCost;

void copyResourceCostAt(int unitId, int index, vector<ResourceCost> &target, const genie::Civ &civ);

bool bothRequirePopulationHeadroom(int unitId, vector<ResourceCost> &resourceCosts, const genie::Civ &civ);

bool isNaturalResourceCost(const ResourceCost &cost);

bool isNaturalResearchResourceCost(const ResearchResourceCost &cost);

bool hasNaturalResourceCost(const genie::Unit &unit);

bool hasNaturalResearchResourceCost(vector<ResearchResourceCost> costs);

string costToString(const vector<ResourceCost> &costs);

string costToString(const vector<ResearchResourceCost> &costs);

int getSumOfNaturalResourceCosts(const vector<ResourceCost>& resourceCosts);

ResearchResourceCost toResearchResourceCost(const ResourceCost &resourceCost);

ResourceCost toResourceCost(const ResearchResourceCost &researchResourceCost);

vector<ResearchResourceCost> toResearchResourceCosts(const vector<ResourceCost> &resourceCosts);

vector<ResourceCost> toResourceCosts(const vector<ResearchResourceCost> &researchResourceCosts);

void multiplyEffect(genie::DatFile *df, uint16_t effectId, uint16_t civ_id, int times);

void multiplyEffectCommand(genie::DatFile *df, genie::EffectCommand& command, uint16_t effectId, uint16_t civ_id, int times);

void multiplyTcAnnexCommand(genie::DatFile *df, genie::EffectCommand& command, int times);

void addDummyAnnexBuilding(genie::DatFile *df, const vector<int16_t> annexUnitIds);


void configureCommunityGamesMod(genie::DatFile *df) {
    addPopulationCostToBombardTower(df);
    addGreatHallTech(df);
    addElitePetard(df);
    modifyCaravanCost(df, 800, 200);
    makeTreesContain200Wood(df);
}

void addPopulationCostToBombardTower(genie::DatFile *df) {
    cout << "Adding population cost to bombard towers of all civs" << endl;
    for (genie::Civ &civ : df->Civs) {
        genie::Unit &bombard_tower = civ.Units.at(BOMBARD_TOWER);
        genie::ResourceUsage<int16_t, int16_t, int16_t> &resourceCosts = bombard_tower.Creatable.ResourceCosts.at(2);
        resourceCosts.Type = TYPE_POPULATION_HEADROOM;
        resourceCosts.Amount = 1;
        resourceCosts.Flag = 0;

        bombard_tower.ResourceStorages.at(0).Type = TYPE_POPULATION_HEADROOM;
        bombard_tower.ResourceStorages.at(0).Amount = -1;
        bombard_tower.ResourceStorages.at(0).Flag = 2;

        bombard_tower.ResourceStorages.at(1).Type = TYPE_CURRENT_POPULATION;
        bombard_tower.ResourceStorages.at(1).Amount = 1;
        bombard_tower.ResourceStorages.at(1).Flag = 2;

        bombard_tower.ResourceStorages.at(2).Type = TYPE_TOTAL_UNITS_OWNED;
        bombard_tower.ResourceStorages.at(2).Amount = 1;
        bombard_tower.ResourceStorages.at(2).Flag = 1;
    }
}

void addGreatHallTech(genie::DatFile *df) {
    cout << "Adding great hall tech" << endl;
    auto effectCommand = new genie::EffectCommand();
    effectCommand->Type = 1; // Resource Modifier
    effectCommand->A = 32; // Resource: Bonus Population Cap
    effectCommand->B = 1; // Mode: +/-
    effectCommand->D = 10; // Amount +/-

    auto effect = new genie::Effect();
    effect->Name = "Great Hall";
    effect->EffectCommands.push_back(*effectCommand);

    df->Effects.push_back(*effect);
    size_t greatHallEffectId = df->Effects.size() - 1;

    auto tech = new genie::Tech();
    tech->Name = "The Great Hall";
    tech->RequiredTechs.push_back(BOMBARD_TOWER_TECH);
    tech->ResearchTime = 5;
    tech->IconID = 103;
    tech->ButtonID = 1;
    tech->EffectID = greatHallEffectId;
    tech->ResearchLocation = BOMBARD_TOWER;

    tech->ResourceCosts.at(0).Type = TYPE_WOOD;
    tech->ResourceCosts.at(0).Amount = 400;
    tech->ResourceCosts.at(0).Flag = 1;

    tech->ResourceCosts.at(1).Type = TYPE_GOLD;
    tech->ResourceCosts.at(1).Amount = 600;
    tech->ResourceCosts.at(1).Flag = 1;

    df->Techs.push_back(*tech);
}

void addElitePetard(genie::DatFile *df) {
    int elitePetardId = -1;
    for (genie::Civ &civ : df->Civs) {
        genie::Unit elitePetard = civ.Units.at(PETARD);
        elitePetard.Enabled = 0;
        elitePetard.Name = "Elite Petard";
        elitePetard.HitPoints = 60;
        elitePetard.Speed = 1.05;
        elitePetard.Type50.DisplayedAttack = 45;
        elitePetard.Type50.BlastWidth = 1.5;
        elitePetard.Type50.BlastAttackLevel = 1;
        elitePetard.Type50.Attacks.at(0).Amount = 150;
        elitePetard.Type50.Attacks.at(1).Amount = 650;
        elitePetard.Type50.Attacks.at(2).Amount = 45;
        elitePetard.Type50.Attacks.at(3).Amount = 70;
        elitePetard.Type50.Attacks.at(4).Amount = 1000;
        elitePetard.Type50.Armours.at(1).Amount = 3;
        elitePetard.Creatable.DisplayedPierceArmour = 3;
        elitePetardId = civ.Units.size();
        cout << "Adding elite petard for civ " << civ.Name << " with ID " << elitePetardId << endl;
        civ.Units.push_back(elitePetard);
        civ.UnitPointers.push_back(1);
    }

    auto effectCommand = new genie::EffectCommand();
    effectCommand->Type = 3; // Upgrade Unit
    effectCommand->A = PETARD;
    effectCommand->B = elitePetardId;

    auto effect = new genie::Effect();
    effect->Name = "Elite Petard";
    effect->EffectCommands.push_back(*effectCommand);
    int elitePetardEffectId = df->Effects.size();
    df->Effects.push_back(*effect);

    auto tech = new genie::Tech();
    tech->Name = "Elite Petard";
    tech->RequiredTechs.push_back(CHEMISTRY);
    tech->RequiredTechCount = 1;
    tech->ResearchTime = 40;
    tech->IconID = 105;
    tech->ButtonID = 9;
    tech->EffectID = elitePetardEffectId;
    tech->ResearchLocation = CASTLE;

    tech->ResourceCosts.at(0).Type = TYPE_FOOD;
    tech->ResourceCosts.at(0).Amount = 450;
    tech->ResourceCosts.at(0).Flag = 1;

    tech->ResourceCosts.at(1).Type = TYPE_GOLD;
    tech->ResourceCosts.at(1).Amount = 450;
    tech->ResourceCosts.at(1).Flag = 1;

    cout << "Adding Elite Petard Tech with id " << df->Techs.size() << endl;
    df->Techs.push_back(*tech);
}

void modifyCaravanCost(genie::DatFile *df, int amountFood, int amountGold) {
    cout << "Setting the cost of Caravan (" << TECH_CARAVAN << ") to " << amountFood << " Food, " << amountGold
         << " Gold" << endl;
    genie::Tech &caravan = df->Techs.at(TECH_CARAVAN);
    caravan.ResourceCosts.at(0).Type = TYPE_FOOD;
    caravan.ResourceCosts.at(0).Amount = amountFood;
    caravan.ResourceCosts.at(0).Flag = 1;

    caravan.ResourceCosts.at(1).Type = TYPE_GOLD;
    caravan.ResourceCosts.at(1).Amount = amountGold;
    caravan.ResourceCosts.at(1).Flag = 1;
}

void makeTreesContain200Wood(genie::DatFile *df) {
    for (genie::Civ &civ : df->Civs) {
        for (genie::Unit &unit: civ.Units) {
            for (auto storage : unit.ResourceStorages) {
                if (storage.Type == TYPE_WOOD && storage.Amount > 50) {
                    cout << "Setting amount of wood in " << unit.Name << " (" << unit.ID << ") to 200" << endl;
                    storage.Amount = 200;
                }
            }
        }
    }
}

void configureExplodingVillagers(genie::DatFile *df) {
    set<int> villagers = {
            ID_FISHING_SHIP,
            ID_TRADE_COG,
            ID_TRADE_CART_EMPTY,
            ID_TRADE_CART_FULL,
            ID_VILLAGER_BASE_M,
            ID_VILLAGER_BASE_F,
            ID_VILLAGER_FARMER_M,
            ID_VILLAGER_FARMER_F,
            ID_VILLAGER_SHEPHERD_M,
            ID_VILLAGER_SHEPHERD_F,
            ID_VILLAGER_FORAGER_M,
            ID_VILLAGER_FORAGER_F,
            ID_VILLAGER_HUNTER_M,
            ID_VILLAGER_HUNTER_F,
            ID_VILLAGER_FISHER_M,
            ID_VILLAGER_FISHER_F,
            ID_VILLAGER_WOOD_M,
            ID_VILLAGER_WOOD_F,
            ID_VILLAGER_GOLD_M,
            ID_VILLAGER_GOLD_F,
            ID_VILLAGER_STONE_M,
            ID_VILLAGER_STONE_F,
            ID_VILLAGER_BUILDER_M,
            ID_VILLAGER_BUILDER_F,
            ID_VILLAGER_REPAIRER_M,
            ID_VILLAGER_REPAIRER_F,
    };

    for (genie::Civ &civ : df->Civs) {
        for (int villager_id : villagers) {
            civ.Units.at(villager_id).DeadUnitID = ID_SABOTEUR;
            cout << "Patched Villager unit " << villager_id << " for civ " << civ.Name << "\n";
        }
        genie::Unit &saboteur = civ.Units.at(ID_SABOTEUR);
        saboteur.HitPoints = 0;
        saboteur.Type50.Attacks.at(0).Amount = 50;
        saboteur.Type50.Attacks.at(1).Amount = 90;
        saboteur.Type50.Attacks.at(2).Amount = 0;
        saboteur.Type50.MaxRange = 2;
        saboteur.Type50.BlastAttackLevel = 1; // cut trees
        saboteur.TrainSound = -1;
        saboteur.WwiseTrainSoundID = 0; // prevent melee unit train sound from playing
        cout << "Patched Saboteur unit for civ " << civ.Name << "\n";
    }
}

void duplicateTechs(genie::DatFile *df, int totalCount) {

    const list<int> techsToDuplicate = {
            YEOMEN,
            EL_DORADO,
            FUROR_CELTICA,
            DRILL,
            MAHOUTS,
            TOWN_WATCH,
            ZEALOTRY,
            ARTILLERY,
            CRENELLATIONS,
            CROP_ROTATION,
            HEAVY_PLOW,
            HORSE_COLLAR,
            GUILDS,
            BANKING,
            ATHEISM,
            LOOM,
            GARLAND_WARS,
            HUSBANDRY,
            FAITH,
            CHEMISTRY,
            CARAVAN,
            BERSERKERGANG,
            MASONRY,
            ARCHITECTURE,
            ROCKETRY,
            TREADMILL_CRANE,
            GOLD_MINING,
            KATAPARUTO,
            LOGISTICA,
            GILLNETS,
            FORGING,
            IRON_CASTING,
            SCALE_MAIL_ARMOR,
            BLAST_FURNACE,
            CHAIN_MAIL_ARMOR,
            PLATE_MAIL_ARMOR,
            PLATE_BARDING_ARMOR,
            SCALE_BARDING_ARMOR,
            CHAIN_BARDING_ARMOR,
            BEARDED_AXE,
            GOLD_SHAFT_MINING,
            FLETCHING,
            BODKIN_ARROW,
            BRACER,
            DOUBLE_BIT_AXE,
            BOW_SAW,
            PADDED_ARCHER_ARMOR,
            LEATHER_ARCHER_ARMOR,
            WHEELBARROW,
            SQUIRES,
            RING_ARCHER_ARMOR,
            TWO_MAN_SAW,
            BLOCK_PRINTING,
            SANCTITY,
            ILLUMINATION,
            HAND_CART,
            FERVOR,
            STONE_MINING,
            STONE_SHAFT_MINING,
            TOWN_PATROL,
            CONSCRIPTION,
            SAPPERS,
            SHIPWRIGHT,
            CAREENING,
            DRY_DOCK,
            SIEGE_ENGINEERS,
            HOARDINGS,
            HEATED_SHOT,
            BLOODLINES,
            PARTHIAN_TACTICS,
            THUMB_RING,
            SUPREMACY,
            HERBAL_MEDICINE,
            SHINKICHON,
            PERFUSION,
            ATLATL,
            WARWOLF,
            GREAT_WALL,
            CHIEFTAINS,
            GREEK_FIRE,
            STRONGHOLD,
            YASAMA,
            OBSIDIAN_ARROWS,
            PANOKSEON,
            KAMANDARAN,
            IRONCLAD,
            SIPAHI,
            INQUISITION,
            CHIVALRY,
            PAVISE,
            SILK_ROAD,
            SULTANS,
            SHATAGNI,
            ORTHODOXY,
            DRUZHINA,
            RECURVE_BOW,
            FABRIC_SHIELDS,
            CARRACK,
            ARQUEBUS,
            ROYAL_HEIRS,
            TORSION_ENGINES,
            TIGUI,
            FARIMBA,
            KASBAH,
            MAGHRABI_CAMELS,
            ARSON,
            ARROWSLITS,
            TUSK_SWORDS,
            DOUBLE_CROSSBOW,
            FORCED_LEVY,
            HOWDAH,
            MANIPUR_CAVALRY,
            CHATRAS,
            PAPER_MONEY,
            STIRRUPS,
            BAGAINS,
            SILK_ARMOR,
            TIMURID_SIEGECRAFT,
            STEPPE_HUSBANDRY,
            CUMAN_MERCENARIES,
            HILL_FORTS,
            TOWER_SHIELDS,
            SUPPLIES,
    };

    for (int techId: techsToDuplicate) {
        genie::Tech tech = df->Techs.at(techId);
        duplicateTech(df, tech, totalCount);
    }

}

void duplicateTech(genie::DatFile *df, const genie::Tech &tech, int totalCount) {
    for (int i = 0; i < (totalCount - 1); ++i) {
        df->Techs.push_back(tech);
    }
    cout << "Added Tech '" << tech.Name << "' for a total of " << totalCount << " instances" << endl;
}

void makeTransportShipsFly(genie::DatFile *df) {
    for (genie::Civ &civ : df->Civs) {
        genie::Unit &transport_ship = civ.Units.at(ID_TRANSPORT_SHIP);
        transport_ship.FlyMode = 1; // true
        transport_ship.TerrainRestriction = 0; // All
        transport_ship.CollisionSize = {0, 0, 0}; // walk through everything, like flying
        cout << "Patched Transport Ship unit for civ " << civ.Name << "\n";
    }
}

void configureKidnap(genie::DatFile *df) {
    const list<int> scoutIds = {
            ID_SCOUT,
            ID_EAGLE_SCOUT,
    };
    for (genie::Civ &civ : df->Civs) {
        for (int scoutId : scoutIds) {
            genie::Unit &scout = civ.Units.at(scoutId);
            cout << "Setting Scout (" << scoutId << ") properties for civ " << civ.Name << endl;
            scout.Type50.Attacks.clear();
            scout.Type50.DisplayedAttack = 0;
            scout.GarrisonCapacity = 1;
            scout.Bird.TaskList.erase(scout.Bird.TaskList.begin());

            cout << "Adding Kidnap Task to scout (" << scoutId << ") for civ " << civ.Name << endl;
            auto kidnapTask = new genie::Task();
            kidnapTask->ActionType = ACTION_KIDNAP_UNIT;
            kidnapTask->ClassID = CLASS_CIVILIAN;
            kidnapTask->WorkRange = 0.25;
            kidnapTask->ProceedingGraphicID = 1966; // SCOUT_AN
            kidnapTask->TargetDiplomacy = 2;
            kidnapTask->GatherType = 2;
            scout.Bird.TaskList.push_back(*kidnapTask);

            cout << "Adding Loot Task to scout (" << scoutId << ") for civ " << civ.Name << endl;
            auto lootTask = new genie::Task();
            lootTask->ActionType = ACTION_LOOT;
            lootTask->ClassID = CLASS_BUILDING;
            lootTask->WorkRange = 0.25;
            lootTask->ProceedingGraphicID = 1966; // SCOUT_AN
            lootTask->TargetDiplomacy = 2;
            lootTask->GatherType = 1;
            scout.Bird.TaskList.push_back(*lootTask);
        }
    }
}

void disableWalls(genie::DatFile *df) {
    const list<int> unitsToDisable = {
            PALISADE_WALL,
    };

    const list<int> techsToDisable = {
            TECH_PALISADE_GATE,
            TECH_STONE_WALLS,
    };

    const list<int> techsToHide = {
            TECH_FORTIFIED_WALL,
    };

    for (int unitId: unitsToDisable) {
        cout << "Disabling unit with id " << unitId << " for all civs" << endl;
        for (genie::Civ &civ : df->Civs) {
            civ.Units.at(unitId).Enabled = 0;
        }
    }

    for (int techId: techsToDisable) {
        genie::Tech *tech = &df->Techs.at(techId);
        cout << "Disabling the effect of tech with id " << techId << " ('" << tech->Name << "')" << endl;
        disableTechEffect(tech);
    }

    for (int techId: techsToHide) {
        genie::Tech *tech = &df->Techs.at(techId);
        cout << "Disabling the research location of tech with id " << techId << " ('" << tech->Name << "')" << endl;
        disableTechResearchLocation(tech);
    }
}

void disableTechEffect(genie::Tech *tech) {
    tech->EffectID = -1;
}

void disableTechResearchLocation(genie::Tech *tech) {
    tech->ResearchLocation = -1;
}

void preventRamsAndSiegeTowersFromBoardingTransportShips(genie::DatFile *df) {
    for (genie::Civ &civ : df->Civs) {
        for (genie::Unit &unit : civ.Units) {
            if (unit.ID == ID_RAM || unit.ID == ID_CAPPED_RAM || unit.ID == ID_SIEGE_RAM || unit.ID == ID_SIEGE_TOWER) {
                list<int> toDelete;
                for (int i = 0; i < unit.Bird.TaskList.size(); i++) {
                    genie::Task task = unit.Bird.TaskList.at(i);
                    if (task.ActionType == ACTION_TYPE_GARRISON && task.ClassID == CLASS_TRANSPORT_BOAT) {
                        toDelete.push_front(i);
                    }
                }

                for (int i:toDelete) {
                    unit.Bird.TaskList.erase(unit.Bird.TaskList.begin() + i);
                }
                cout << "Patched unit " << unit.Name << " (" << unit.ID << ") unit for civ " << civ.Name << "\n";
            }
        }
    }
}

ResearchResourceCost toResearchResourceCost(const ResourceCost &resourceCost) {
    ResearchResourceCost researchResourceCost;
    researchResourceCost.Type = resourceCost.Type;
    researchResourceCost.Amount = resourceCost.Amount;
    researchResourceCost.Flag = (bool) resourceCost.Flag;
    return researchResourceCost;
}

ResourceCost toResourceCost(const ResearchResourceCost &researchResourceCost) {
    ResourceCost resourceCost;
    resourceCost.Type = researchResourceCost.Type;
    resourceCost.Amount = researchResourceCost.Amount;
    resourceCost.Flag = (bool) researchResourceCost.Flag;
    return resourceCost;
}


vector<ResearchResourceCost> toResearchResourceCosts(const vector<ResourceCost> &resourceCosts) {
    vector<ResearchResourceCost> researchResourceCosts;
    researchResourceCosts.reserve(resourceCosts.size());
    for (const ResourceCost &resourceCost : resourceCosts) {
        researchResourceCosts.push_back(toResearchResourceCost(resourceCost));
    }
    return researchResourceCosts;
}

vector<ResourceCost> toResourceCosts(const vector<ResearchResourceCost> &researchResourceCosts) {
    vector<ResourceCost> resourceCosts;
    resourceCosts.reserve(researchResourceCosts.size());
    for (const ResearchResourceCost &researchResourceCost : researchResourceCosts) {
        resourceCosts.push_back(toResourceCost(researchResourceCost));
    }
    return resourceCosts;
}


void jumbleCosts(genie::DatFile *df) {
    vector<int> unitIds;
    for (genie::Unit unit : df->Civs.at(0).Units) {
        if (hasNaturalResourceCost(unit)) {
            unitIds.push_back(unit.ID);
        }
    }

    vector<int> techIds;
    size_t index = 0;
    for (const genie::Tech &tech : df->Techs) {
        vector<ResearchResourceCost> resourceCopy = tech.ResourceCosts;
        if (hasNaturalResearchResourceCost(resourceCopy)) {
            techIds.push_back(index);
        }
        index++;
    }

    vector<vector<ResourceCost>> allTheCosts;
    for (int unitId: unitIds) {
        vector<ResourceCost> resourceCopy = df->Civs.at(0).Units.at(unitId).Creatable.ResourceCosts;
        allTheCosts.push_back(resourceCopy);
    }

    for (int techId: techIds) {
        vector<ResourceCost> resourceCopy = toResourceCosts(df->Techs.at(techId).ResourceCosts);
        allTheCosts.push_back(resourceCopy);
    }

    for (int i = 0; i < 10; i++) {
        unsigned int seed = std::random_device()();
        cout << "Seed for shuffling unit and tech costs is: " << to_string(seed) << endl;
        shuffle(allTheCosts.begin(), allTheCosts.end(), std::mt19937(seed));

        size_t costIndex = 0;
        for (int techId: techIds) {
            vector<ResearchResourceCost> researchResourceCosts = toResearchResourceCosts(allTheCosts.at(costIndex));
            cout << "Setting cost of tech with id " << techId << " to " << costToString(researchResourceCosts) << endl;
            df->Techs.at(techId).ResourceCosts = researchResourceCosts;
            costIndex++;
        }
        for (int unitId: unitIds) {
            vector<ResourceCost> &resourceCosts = allTheCosts.at(costIndex);
            cout << "Setting cost of unit with id " << unitId << " to " << costToString(resourceCosts) << endl;
            if (!bothRequirePopulationHeadroom(unitId, resourceCosts, df->Civs.at(0))) {
                copyResourceCostAt(unitId, 2, resourceCosts, df->Civs.at(0));
            }
            for (genie::Civ &civ : df->Civs) {
                civ.Units.at(unitId).Creatable.ResourceCosts = resourceCosts;
            }
            costIndex++;
        }

        int maleVillagerCost = getSumOfNaturalResourceCosts(
                df->Civs.at(0).Units.at(ID_VILLAGER_BASE_M).Creatable.ResourceCosts);
        int femaleVillagerCost = getSumOfNaturalResourceCosts(
                df->Civs.at(0).Units.at(ID_VILLAGER_BASE_F).Creatable.ResourceCosts);
        if (maleVillagerCost <= 200 && femaleVillagerCost <= 200) {
            return;
        }
        cout << "Villagers are too expensive, reshuffling…" << endl;
    }
    cout << "Giving up, villagers stay expensive, sorry." << endl;
}

void jumbleUnitCosts(genie::DatFile *df) {
    vector<int> unitIds;

    for (genie::Unit unit : df->Civs.at(0).Units) {
        if (hasNaturalResourceCost(unit)) {
            unitIds.push_back(unit.ID);
        }
    }

    vector<vector<ResourceCost>> allTheCosts;
    for (int unitId: unitIds) {
        vector<ResourceCost> resourceCopy = df->Civs.at(0).Units.at(unitId).Creatable.ResourceCosts;
        allTheCosts.push_back(resourceCopy);
    }

    for (int i = 0; i < 10; i++) {
        unsigned int seed = std::random_device()();
        cout << "Seed for shuffling unit costs is: " << to_string(seed) << endl;
        shuffle(allTheCosts.begin(), allTheCosts.end(), std::mt19937(seed));

        size_t index = 0;
        for (int unitId: unitIds) {
            vector<ResourceCost> &resourceCosts = allTheCosts.at(index);
            cout << "Setting cost of unit with id " << unitId << " to " << costToString(resourceCosts) << endl;
            if (!bothRequirePopulationHeadroom(unitId, resourceCosts, df->Civs.at(0))) {
                copyResourceCostAt(unitId, 2, resourceCosts, df->Civs.at(0));
            }
            for (genie::Civ &civ : df->Civs) {
                civ.Units.at(unitId).Creatable.ResourceCosts = resourceCosts;
            }
            index++;
        }

        int maleVillagerCost = getSumOfNaturalResourceCosts(
                df->Civs.at(0).Units.at(ID_VILLAGER_BASE_M).Creatable.ResourceCosts);
        int femaleVillagerCost = getSumOfNaturalResourceCosts(
                df->Civs.at(0).Units.at(ID_VILLAGER_BASE_F).Creatable.ResourceCosts);
        if (maleVillagerCost <= 200 && femaleVillagerCost <= 200) {
            return;
        }
        cout << "Villagers are too expensive, reshuffling…" << endl;
    }
    cout << "Giving up, villagers stay expensive, sorry." << endl;
}

void jumbleTechCosts(genie::DatFile *df) {
    vector<int> techIds;

    vector<vector<ResearchResourceCost>> allTheCosts;
    size_t index = 0;
    for (const genie::Tech &tech : df->Techs) {
        vector<ResearchResourceCost> resourceCopy = tech.ResourceCosts;
        if (hasNaturalResearchResourceCost(resourceCopy)) {
            allTheCosts.push_back(resourceCopy);
            techIds.push_back(index);
        }
        index++;
    }

    unsigned int seed = std::random_device()();
    cout << "Seed for shuffling tech costs is: " << to_string(seed) << endl;
    shuffle(allTheCosts.begin(), allTheCosts.end(), std::mt19937(seed));

    index = 0;
    for (size_t techId : techIds) {
        vector<ResearchResourceCost> &resourceCosts = allTheCosts.at(index);
        cout << "Setting cost of tech with id " << techId << " to " << costToString(resourceCosts) << endl;
        df->Techs.at(techId).ResourceCosts = resourceCosts;
        index++;
    }

}

int getSumOfNaturalResourceCosts(const vector<ResourceCost>& resourceCosts) {
    int16_t sum = 0;
    for (const ResourceCost& cost : resourceCosts) {
        if (cost.Type > -1 && cost.Type < 4) {
            sum += cost.Amount;
        }
    }
    return sum;
}

bool isNaturalResourceCost(const ResourceCost &cost) {
    return cost.Type != -1 && cost.Type < 4 && cost.Amount > 0;
}

bool isNaturalResearchResourceCost(const ResearchResourceCost &cost) {
    return cost.Type != -1 && cost.Type < 4 && cost.Amount > 0;
}

bool hasNaturalResourceCost(const genie::Unit &unit) {
    vector<ResourceCost> costs = unit.Creatable.ResourceCosts;
    return any_of(costs.begin(), costs.end(), isNaturalResourceCost);
}

bool hasNaturalResearchResourceCost(vector<ResearchResourceCost> costs) {
    return any_of(costs.begin(), costs.end(), isNaturalResearchResourceCost);
}

string costToString(const vector<ResourceCost> &costs) {
    string s;
    for (const ResourceCost &cost : costs) {
        if (cost.Flag == 1) {
            s += to_string(cost.Amount);
            s += " ";
            switch (cost.Type) {
                case TYPE_FOOD:
                    s += "Food ";
                    break;
                case TYPE_WOOD:
                    s += "Wood ";
                    break;
                case TYPE_GOLD:
                    s += "Gold ";
                    break;
                case TYPE_STONE:
                    s += "Stone ";
                    break;
                case TYPE_POPULATION_HEADROOM:
                    s += "Pop ";
                    break;
                default:
                    s += "vat?";
            }
        }
    }
    return s;
}

string costToString(const vector<ResearchResourceCost> &costs) {
    string s;
    for (const ResearchResourceCost &cost : costs) {
        if (cost.Flag == 1) {
            s += to_string(cost.Amount);
            s += " ";
            switch (cost.Type) {
                case TYPE_FOOD:
                    s += "Food ";
                    break;
                case TYPE_WOOD:
                    s += "Wood ";
                    break;
                case TYPE_GOLD:
                    s += "Gold ";
                    break;
                case TYPE_STONE:
                    s += "Stone ";
                    break;
                case TYPE_POPULATION_HEADROOM:
                    s += "Pop ";
                    break;
                default:
                    s += "vat?";
            }
        }
    }
    return s;
}

bool bothRequirePopulationHeadroom(int unitId, vector<ResourceCost> &resourceCosts, const genie::Civ &civ) {
    return ((civ.Units.at(unitId).Creatable.ResourceCosts.at(2).Type == TYPE_POPULATION_HEADROOM &&
             resourceCosts.at(2).Type == TYPE_POPULATION_HEADROOM) ||
            (civ.Units.at(unitId).Creatable.ResourceCosts.at(2).Type != TYPE_POPULATION_HEADROOM &&
             resourceCosts.at(2).Type != TYPE_POPULATION_HEADROOM));
}

void copyResourceCostAt(int unitId, int index, vector<ResourceCost> &target, const genie::Civ &civ) {
    genie::ResourceUsage<int16_t, int16_t, int16_t> source = civ.Units.at(unitId).Creatable.ResourceCosts.at(index);
    target.at(index).Type = source.Type;
    target.at(index).Amount = source.Amount;
    target.at(index).Flag = source.Flag;
}

void debugPrintEffect(genie::DatFile *df, uint16_t effectId) {
    const genie::Effect &effect = df->Effects.at(effectId);
    cout << effectId << " - " << effect.Name << endl;
}

void multiplyCivilizationBonuses(genie::DatFile *df, int times) {
    std::vector<uint16_t> effectIds;
    for (auto it = df->Civs.begin(); it != df->Civs.end(); ++it) {
        auto civ_id = it - df->Civs.begin();
        if (civ_id == 0) // skip gaia
            continue;

        effectIds.push_back(it->TechTreeID);
        effectIds.push_back(it->TeamBonusID);

        cout << "From civ " << it - df->Civs.begin() << " " << it->Name << " " << it->Name2 << ":" << endl;
        multiplyEffect(df, it->TechTreeID, civ_id, times);
        multiplyEffect(df, it->TeamBonusID, civ_id, times);
        cout << endl;
    }

    for (auto it = df->Techs.begin(); it != df->Techs.end(); ++it) {
        if (it->Civ == 0 || it->Civ == -1 || it->EffectID == -1)
            continue;
        if (find(effectIds.begin(), effectIds.end(), it->EffectID) != effectIds.end())
            continue;

        effectIds.push_back(it->EffectID);

        cout << "From tech " << it->Name << " " << it->Name2 << "(" << df->Civs[it->Civ].Name << ")" << ":" << endl;
        multiplyEffect(df, it->EffectID, it->Civ, times);
        cout << endl;
    }
}

int capEffectMultiplication(uint16_t effectId, int times) {
    // Cap some effects multiplication so it doesnt break the gameplay too much
    // especially for higher multiplication values
    static const map<uint16_t, int> CAPPED_EFFECTS {
        // prevent huns from starting with less wood since the "no houses" bonus cant be multiplied
        {EFFECT_ID_HUNS_100_WOOD, 1},
        // avoid persian tc and dock hitpoints from going over hitpoints limit
        {EFFECT_ID_PERSIAN_TC_HITPOINTS, 4},
        {EFFECT_ID_PERSIAN_DOCK_HITPOINTS, 5}
    };

    auto it = CAPPED_EFFECTS.find(effectId);
    if (it != CAPPED_EFFECTS.end()) {
        return it->second;
    }
    return times;
}

void multiplyEffect(genie::DatFile *df, uint16_t effectId, uint16_t civ_id, int times) {
    times = capEffectMultiplication(effectId, times);

    auto& effect = df->Effects.at(effectId);
    cout << effectId << " - " << effect.Name << endl;
    for (auto& command: effect.EffectCommands) {
        multiplyEffectCommand(df, command, effectId, civ_id, times);
    }
}

void multiplyEffectCommand(genie::DatFile *df, genie::EffectCommand& command, uint16_t effectId, uint16_t civ_id, int times) {
    string commandTypeName = "UNK";
    if (command.Type == COMMAND_RESOURCE_MODIFIER) {
        commandTypeName = "Resource Modifier (Set/+/-)";
    }
    else if (command.Type == COMMAND_UPGRADE_UNIT) {
        const string fromUnit = df->Civs[0].Units[command.A].Name;
        const string toUnit = df->Civs[0].Units[command.B].Name;
        commandTypeName = "Upgrade Unit " + fromUnit + "(" + to_string(command.A) + ") -> " + toUnit + "(" + to_string(command.B) + ")";
    }
    else if (command.Type == COMMAND_ATTRIBUTE_MODIFIER) {
        commandTypeName = "Attribute Modifier (+/-)";
    }
    else if (command.Type == COMMAND_ATTRIBUTE_MULTIPLIER) {
        commandTypeName = "Attribute Modifier (Mult)";
    }
    else if (command.Type == COMMAND_TEAM_ATTRIBUTE_MODIFIER) {
        commandTypeName = "Team Attribute Modifier (Set)";
    }
    else if (command.Type == COMMAND_TECH_COST_MODIFIER) {
        string techName = command.A > 0 ? df->Techs[command.A].Name : "-1";
        commandTypeName = "Tech Cost Modifier (Set/+/-) " + techName;
    }
    else if (command.Type == COMMAND_DISABLE_TECH) {
        string techName = command.D > 0 ? df->Techs[command.D].Name : "-1";
        commandTypeName = "Disable tech " + techName;
    }
    else if (command.Type == COMMAND_TECH_TIME_MODIFIER) {
        string techName = command.A > 0 ? df->Techs[command.A].Name : "-1";
        commandTypeName = "Tech Time Modifier (Set/+/-) " + techName;
    }

    float oldD = command.D;
    switch (command.Type) {
        case COMMAND_RESOURCE_MODIFIER:
            if (command.B == 0 && command.D < 7e13) { // Set
                auto baseValue = df->Civs[civ_id].Resources.at(command.A);
                if (command.A == RESOURCE_RESEARCH_COST_MODIFIER) {
                    baseValue = 1.0;
                }
                cout << "Calculating new resource modifier (set): baseValue=" << baseValue << " command.D=" << command.D << " times=" << times << endl;
                command.D = (command.D - baseValue) * times + baseValue;
            } else if (command.B == 1) { // + or -
                command.D *= times;
            }
            break;
        case COMMAND_UPGRADE_UNIT:
            if (command.A == ID_EMPTY_TC_ANNEX) {
                multiplyTcAnnexCommand(df, command, times);
            }
            break;
        case COMMAND_ATTRIBUTE_MODIFIER:
            if (command.C == 8 || command.C == 9) { // Armor/Atack
                int v = command.D;
                int armor_type = v / 256;
                int amount = v % 256;
                command.D = float(armor_type * 256 + min(amount * times, 255));
            }
            else {
                command.D *= times;
            }
            break;
        case COMMAND_ATTRIBUTE_MULTIPLIER:
            command.D = pow(command.D, times);
            // make sure units wont have too many hitpoints
            if (command.A != -1) {
                if (command.C == 0) {
                    const int unitHitPoints = df->Civs[civ_id].Units[command.A].HitPoints;
                    if (unitHitPoints * command.D >= (1 << 16)) {
                        cout << "[WARNING] unit with too many hitpoints: " << df->Civs[civ_id].Units[command.A].Name << " (" << df->Civs[civ_id].Name << ")" << endl;
                    }
                }
            }

            break;
        case COMMAND_TEAM_ATTRIBUTE_MODIFIER:
            if (command.A != -1) {
                float delta = command.D - df->Civs[civ_id].Units[command.A].ResourceStorages[0].Amount;
                command.D = delta * times;
            }
            break;
        case COMMAND_TECH_COST_MODIFIER: // Tech Cost Modifier (Set/+/-)
            if (command.C == 0) { // Set
                // TODO: do something clever
            } else if (command.C == 1) { // + or -
                command.D *= times;
            }
            break;
    }
    if (command.D == oldD) {
        cout << "[UNCHANGED] ";
    }
    cout << "command type: " << int(command.Type) << " - " << commandTypeName << " - " << "(A=" << command.A << " B=" << command.B << " C=" << command.C << " D=" << oldD << ") => " << command.D << endl;
}

void multiplyTcAnnexCommand(genie::DatFile *df, genie::EffectCommand& command, int times) {
    int total_leafs = 0;

    // store unit ids for every created tc annex node
    vector<int16_t> annexUnits;
    annexUnits.push_back(command.B);
    
    int leafCount = 1;

    // find the subtree that will produce less than `times` leaves with the maximum degree (4)
    while (leafCount * 4 < times) {
        addDummyAnnexBuilding(
            df,
            vector<int16_t>(4, annexUnits.back())
        );
        annexUnits.push_back(df->Civs[0].Units.size() - 1);
        leafCount *= 4;
    }

    // find how many of each annex unit types are needed to reach `times` replicas
    // add them in a tree using a stack
    vector<int16_t> stack;
    for (int i = annexUnits.size() - 1; i >= 0; --i) {
        stack.insert(stack.end(), times / leafCount, annexUnits[i]);
        times %= leafCount;
        leafCount /= 4; // descend one level
        while (stack.size() >= 4) {
            addDummyAnnexBuilding(df, stack);
            stack.erase(stack.begin(), stack.begin()+4);
            stack.push_back(df->Civs[0].Units.size() - 1);
        }
    }
    if (stack.size() != 1) {
        addDummyAnnexBuilding(df, stack);
        stack.clear();
        stack.push_back(df->Civs[0].Units.size() - 1);
    }
    command.B = stack.at(0);
}

void addDummyAnnexBuilding(genie::DatFile *df, const vector<int16_t> annexUnitIds) {
    genie::Unit dummyAnnexUnit(df->Civs[0].Units[annexUnitIds[0]]);
    dummyAnnexUnit.Building.Annexes.clear();

    const std::vector<std::pair<int, int>> misplacements {
        {0, 0},
        {-1, 0},
        {-1, 1},
        {0, 1}
    };

    // generate the annex instances with the given unit ids
    for (int i = 0; i < 4; ++i) {
        auto newAnnex = genie::unit::BuildingAnnex();
        newAnnex.Misplacement = misplacements[i];
        newAnnex.UnitID = i < annexUnitIds.size() ? annexUnitIds.at(i) : -1;
        dummyAnnexUnit.Building.Annexes.push_back(newAnnex);
    }
    for (auto& civ : df->Civs) {
        civ.Units.push_back(dummyAnnexUnit);
        civ.UnitPointers.push_back(1);
    }
}

