/*
  Copyright (C) 2012-2013 - Voidious

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <iostream>
#include <exception>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sstream>
#include "shapes.h"
#include "bbconst.h"
#include "bbutil.h"
#include "bbpigfx.h"
#include "filemanager.h"
#include "stage.h"
#include "bbengine.h"
#include "replaybuilder.h"
#include "gfxeventhandler.h"
#include "cliprinthandler.h"
#include "clipackagereporter.h"
#include "tarzipper.h"
#include "ResourcePath.hpp"

void printUsage() {
  std::cout << "Usage:" << std::endl;
  std::cout << "  ./berrybots [-nodisplay] [-savereplay]"
            << " <stage.lua> <bot1.lua> [<bot2.lua> ...]" << std::endl;
  std::cout << "  OR" << std::endl;
  std::cout << "  ./berrybots -packstage <stage.lua> <version>"
            << std::endl;
  std::cout << "  OR" << std::endl;
  std::cout << "  ./berrybots -packbot <bot.lua> <version>"
            << std::endl;
  exit(0);
}

char* replayFilename(const char *stageName) {
  std::stringstream nameStream;
  char *timestamp = getTimestamp();
  nameStream << ((stageName == 0) ? "unknown" : stageName) << "-"
             << timestamp << ".html";
  delete timestamp;

  std::string filename = nameStream.str();
  char *newFilename = new char[filename.length() + 1];
  strcpy(newFilename, filename.c_str());
  
  return newFilename;
}

int main(int argc, char *argv[]) {
  Zipper *zipper = new TarZipper();
  FileManager *fileManager = new FileManager(zipper);
  CliPackageReporter *packageReporter = new CliPackageReporter();
  fileManager->setListener(packageReporter);

  char *shipsBaseDir = fileManager->getAbsFilePath(SHIPS_SUBDIR);
  char *stagesBaseDir = fileManager->getAbsFilePath(STAGES_SUBDIR);

  if (flagExists(argc, argv, "packstage")) {
    char **stageInfo = parseFlag(argc, argv, "packstage", 2);
    if (stageInfo == 0) {
      printUsage();
    } else {
      // TODO: add a new flag for obfuscating source code
      bool obfuscate = false;
      try {
        char *stageAbsName = fileManager->getAbsFilePath(stageInfo[0]);
        char *stageName =
            fileManager->parseRelativeFilePath(stagesBaseDir, stageAbsName);
        if (stageName == 0) {
          std::cout << "Stage must be located under " << STAGES_SUBDIR
                    << "/ subdirectory: " << stageInfo[0] << std::endl;
        } else {
          fileManager->packageStage(stagesBaseDir, stageName, stageInfo[1],
                                    CACHE_SUBDIR, TMP_SUBDIR, obfuscate, true);
          delete stageName;
        }
        delete stageAbsName;
      } catch (std::exception *e) {
        std::cout << "BerryBots encountered an error:" << std::endl;
        std::cout << "  " << e->what() << std::endl;
        delete e;
      }
      delete stageInfo;
    }
    return 0;
  }

  if (flagExists(argc, argv, "packbot")) {
    char **shipInfo = parseFlag(argc, argv, "packbot", 2);
    if (shipInfo == 0) {
      printUsage();
    } else {
      // TODO: add a new flag for obfuscating source code
      bool obfuscate = false;
      try {
        char *shipAbsName = fileManager->getAbsFilePath(shipInfo[0]);
        char *shipName =
           fileManager->parseRelativeFilePath(shipsBaseDir, shipAbsName);
        if (shipName == 0) {
          std::cout << "Ship must be located under " << SHIPS_SUBDIR
                    << "/ subdirectory: " << shipInfo[0] << std::endl;
        } else {
          fileManager->packageShip(shipsBaseDir, shipName, shipInfo[1],
                                   CACHE_SUBDIR, TMP_SUBDIR, obfuscate, true);
          delete shipName;
        }
        delete shipAbsName;
      } catch (std::exception *e) {
        std::cout << "BerryBots encountered an error:" << std::endl;
        std::cout << "  " << e->what() << std::endl;
        delete e;
      }
      delete shipInfo;
    }
    return 0;
  }

  bool nodisplay = flagExists(argc, argv, "nodisplay");
  bool saveReplay = flagExists(argc, argv, "savereplay");
  int optArgsOffset = (nodisplay ? 1 : 0) + (saveReplay ? 1 : 0);
  if (argc < 3 + optArgsOffset) {
    printUsage();
  }

  srand(time(NULL));
  int screenWidth;
  int screenHeight;
  init(&screenWidth, &screenHeight);

  CliPrintHandler *printHandler = new CliPrintHandler();

  BerryBotsEngine *engine =
      new BerryBotsEngine(printHandler, fileManager, resourcePath().c_str());
  Stage *stage = engine->getStage();
  // TODO: Enable graphical debugging on Raspberry Pi. Main barrier is UI.
  stage->disableUserGfx();

  char *stageAbsName = fileManager->getAbsFilePath(argv[1 + optArgsOffset]);
  char *stageName =
      fileManager->parseRelativeFilePath(stagesBaseDir, stageAbsName);
  if (stageName == 0) {
    std::cout << "Stage must be located under " << STAGES_SUBDIR
              << "/ subdirectory: " << argv[1 + optArgsOffset] << std::endl;
    return 0;
  }
  try {
    engine->initStage(stagesBaseDir, stageName, CACHE_SUBDIR);
  } catch (EngineException *e) {
    delete stageAbsName;
    delete stageName;
    std::cout << "BerryBots initialization failed:" << std::endl;
    std::cout << "  " << e->what() << std::endl;
    delete e;
    return 0;
  }
  delete stageAbsName;
  delete stageName;

  int firstTeam = (2 + optArgsOffset);
  int numTeams = argc - firstTeam;
  char **teams = new char*[numTeams];
  for (int x = 0; x < numTeams; x++) {
    char *teamAbsName = fileManager->getAbsFilePath(argv[x + firstTeam]);
    char *teamName =
        fileManager->parseRelativeFilePath(shipsBaseDir, teamAbsName);
    if (teamName == 0) {
      std::cout << "Ship must be located under " << SHIPS_SUBDIR
                << "/ subdirectory: " << argv[x + firstTeam] << std::endl;
      return 0;
    }
    teams[x] = teamName;
    delete teamAbsName;
  }

  printHandler->setNumTeams(numTeams);
  try {
    engine->initShips(shipsBaseDir, teams, numTeams, CACHE_SUBDIR);
  } catch (EngineException *e) {
    std::cout << "BerryBots initialization failed:" << std::endl;
    std::cout << "  " << e->what() << std::endl;
    delete e;
    return 0;
  }

  printHandler->updateTeams(engine->getTeams());

  GfxEventHandler *gfxHandler = 0;
  if (!nodisplay) {
    gfxHandler = new GfxEventHandler();
    stage->addEventHandler((EventHandler*) gfxHandler);
    initVgGfx(screenWidth, screenHeight, stage, engine->getShips(),
        engine->getNumShips());
    drawGame(screenWidth, screenHeight, stage, engine->getShips(),
        engine->getNumShips(), engine->getGameTime(), gfxHandler);
  }

  time_t realTime1;
  time_t realTime2;
  time(&realTime1);
  int realSeconds = 0;

  try {
    do {
      engine->processTick();
      if (!nodisplay) {
        drawGame(screenWidth, screenHeight, stage, engine->getShips(),
            engine->getNumShips(), engine->getGameTime(), gfxHandler);
      }
  
      time(&realTime2);
      if (realTime2 - realTime1 > 0) {
        realSeconds++;
        if (realSeconds % 10 == 0) {
          std::cout << "TPS: "
                    << (((double) engine->getGameTime()) / realSeconds)
                    << std::endl;
        }
      }
      realTime1 = realTime2;
    } while (!engine->isGameOver());
  } catch (EngineException *e) {
    std::cout << "BerryBots encountered an error:" << std::endl;
    std::cout << "  " << e->what() << std::endl;
    delete e;
    return 0;
  }

  if (!nodisplay) {
    destroyVgGfx();
    finish();
  }

  const char* winnerName = engine->getWinnerName();
  if (winnerName != 0) {
    std::cout << std::endl<< winnerName << " wins! Congratulations!"
              << std::endl;
  }

  std::cout << std::endl << "Results:" << std::endl;
  Team **rankedTeams = engine->getRankedTeams();
  bool hasScores = false;
  for (int x = 0; x < numTeams; x++) {
    if (rankedTeams[x]->result.score != 0) {
      hasScores = true;
      break;
    }
  }
  TeamResult *firstResult = &(rankedTeams[0]->result);
  int numStats = firstResult->numStats;
  char **statKeys = 0;
  if (numStats > 0) {
    statKeys = new char*[firstResult->numStats];
    for (int x = 0; x < numStats; x++) {
      statKeys[x] = new char[strlen(firstResult->stats[x]->key) + 1];
      strcpy(statKeys[x], firstResult->stats[x]->key);
    }
  }

  for (int x = 0; x < engine->getNumTeams(); x++) {
    TeamResult *result = &(rankedTeams[x]->result);
    if (result->showResult) {
      std::cout << "    " << rankedTeams[x]->name << ":" << std::endl;
      std::cout << "        Rank: ";
      if (result->rank == 0) {
        std::cout << "-";
      } else {
        std::cout << result->rank;
      }
      std::cout << std::endl;
      if (hasScores) {
        std::cout << "        Score: " << round(result->score, 2) << std::endl;
      }

      for (int y = 0; y < numStats; y++) {
        char *key = statKeys[y];
        bool found = false;
        for (int z = 0; z < result->numStats; z++) {
          char *resultKey = result->stats[z]->key;
          if (strcmp(key, resultKey) == 0) {
            std::cout << "        " << key << ": "
                      << round(result->stats[z]->value, 2) << std::endl;
            found = true;
            break;
          }
        }
        if (!found) {
          std::cout << "        " << key << ": -" << std::endl;
        }
      }
    }
  }

  std::cout << std::endl << "CPU time used per tick (microseconds):"
            << std::endl;
  for (int x = 0; x < engine->getNumTeams(); x++) {
    Team *team = engine->getTeam(x);
    if (!team->stageShip && !team->disabled) {
      std::cout << "  " << team->name << ": "
                << (team->totalCpuTime / team->totalCpuTicks) << std::endl;
    }
  }

  if (realSeconds > 0) {
    std::cout << std::endl << "TPS: "
              << (((double) engine->getGameTime()) / realSeconds) << std::endl;
  }

  if (saveReplay) {
    ReplayBuilder *replayBuilder = engine->getReplayBuilder();

    // TODO: move this into a function in the engine
    Team **rankedTeams = engine->getRankedTeams();
    replayBuilder->setResults(rankedTeams, engine->getNumTeams());

    char *filename = 0;
    char *absFilename = 0;
    do {
      if (filename != 0) {
        delete filename;
      }
      if (absFilename != 0) {
        delete absFilename;
      }
      filename = replayFilename(stage->getName());
      char *filePath = fileManager->getFilePath(REPLAYS_SUBDIR, filename);
      absFilename = fileManager->getAbsFilePath(filePath);
      delete filePath;
    } while (fileManager->fileExists(absFilename));
    replayBuilder->saveReplay(filename);
    std::cout << std::endl << "Saved replay to: " << REPLAYS_SUBDIR << "/"
              << filename << std::endl;
    delete filename;
    delete absFilename;
  }

  std::cout << std::endl;

  delete engine;
  for (int x = 0; x < numTeams; x++) {
    delete teams[x];
  }
  delete teams;
  delete rankedTeams;
  for (int x = 0; x < numStats; x++) {
    delete statKeys[x];
  }
  delete statKeys;
  delete printHandler;
  delete packageReporter;
  delete fileManager;
  delete zipper;
  delete shipsBaseDir;
  delete stagesBaseDir;

  return 0;
}
