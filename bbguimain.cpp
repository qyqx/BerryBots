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
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sstream>
#include <math.h>
#include <SFML/Graphics.hpp>
#include <wx/wx.h>

#include "bbwx.h"
#include "stage.h"
#include "bbengine.h"
#include "printhandler.h"
#include "guimanager.h"
#include "bbutil.h"
#include "basedir.h"
#include "filemanager.h"
#include "sysexec.h"

using namespace std;

PrintHandler *printHandler = 0;

class BerryBotsApp : public wxApp {
  GuiManager *guiManager_;
  GuiListener *guiListener_;
  FileManager *fileManager_;
  SystemExecutor *systemExecutor_;

  public:
    virtual bool OnInit();
    virtual void OnQuit(wxCommandEvent &event);
    void quit();
    virtual void OnNewMatch(wxCommandEvent &event);
    virtual void OnPackageShip(wxCommandEvent &event);
    virtual void OnPackageStage(wxCommandEvent &event);
    virtual void OnGameRunner(wxCommandEvent &event);
    virtual void OnErrorConsole(wxCommandEvent &event);
    virtual void MacReopenApp();
    void onChangeBaseDir(wxCommandEvent &event);
    void onBrowseStages(wxCommandEvent &event);
    void onBrowseShips(wxCommandEvent &event);
    void onBrowseRunners(wxCommandEvent &event);
    void onBrowseReplays(wxCommandEvent &event);
    void onBrowseApidocs(wxCommandEvent &event);
  private:
    std::string dirName(const char *name);
};

class AppGuiListener : public GuiListener {
  BerryBotsApp *app_;
  
  public:
    AppGuiListener(BerryBotsApp *app);
    ~AppGuiListener();
    void onAllWindowsClosed();
};

wxIMPLEMENT_APP(BerryBotsApp);

bool BerryBotsApp::OnInit() {
#ifndef __WXOSX__
  // Needed to load stage previews and wxWidgets dialog icons.
  wxImage::AddHandler(new wxPNGHandler);
#endif

#ifdef __WXOSX__
  // On OS X, it complains if we initialize our first SFML window after the
  // wxWidgets windows have set their menu bars or after the base dir selector,
  // so initialize one here first.
  sf::RenderWindow *window = new sf::RenderWindow(
      sf::VideoMode(800, 600), "BerryBots", sf::Style::Default,
      sf::ContextSettings(0, 0, 0, 2, 0));
  delete window;
#endif

#ifndef __WINDOWS__
  // On Mac OS X and Linux, if a base directory has not yet been selected, show
  // an informational dialog before the file chooser comes up.
  if (!isConfigured()) {
    std::stringstream configInfo;
    configInfo << "TLDR: Before you can use BerryBots, you need to select a "
               << "base " << DIRECTORY << ". This is where BerryBots will store "
               << "files like ships and stages."
               << std::endl << std::endl
#ifdef __WXGTK__
               << "~/Documents/BerryBots is a reasonable choice."
#else
               << "Documents > BerryBots is a reasonable choice."
#endif
               << std::endl << std::endl
               << "--------"
               << std::endl << std::endl
               << "After selecting a " << DIRECTORY << ", sub" << DIRECTORIES
               << " will be created for ships (" << dirName("bots") << "), "
               << "stages (" << dirName("stages") << "), and Game Runners "
               << "(" << dirName("runners") << "). The samples will be copied "
               << "into these " << DIRECTORIES << ". Place your own programs "
               << "in them, too."
               << std::endl << std::endl
#ifdef __WXOSX__
               // On Linux, we use the read-only copy in /usr/share/berrybots.
               << "The BerryBots Lua API documentation will be copied into the "
               << dirName("apidoc") << " sub" << DIRECTORY << "."
               << std::endl << std::endl
#endif
               << "As needed, BerryBots will also create a cache sub"
               << DIRECTORY << " (" << dirName("cache") << ") for unpackaged "
               << "ships and stages, a replays sub" << DIRECTORY << " ("
               << dirName("replays") << "), and a temp sub" << DIRECTORY << " ("
               << dirName(".tmp") << ") for working files used by BerryBots."
               << std::endl << std::endl
               << "Have fun!";

    wxMessageDialog selectBaseDirMessage(NULL, configInfo.str(),
                                         "BerryBots Setup", wxOK);
    if (selectBaseDirMessage.ShowModal() != wxID_OK) {
      return false;
    }
  }
#endif

  fileManager_->recursiveDelete(getTmpDir().c_str());  
  guiListener_ = new AppGuiListener(this);
  guiManager_ = new GuiManager(guiListener_);
  fileManager_ = new FileManager();
  systemExecutor_ = new SystemExecutor();

  Connect(wxID_EXIT, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(BerryBotsApp::OnQuit));
  Connect(NEW_MATCH_MENU_ID, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(BerryBotsApp::OnNewMatch));
  Connect(PACKAGE_SHIP_MENU_ID, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(BerryBotsApp::OnPackageShip));
  Connect(PACKAGE_STAGE_MENU_ID, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(BerryBotsApp::OnPackageStage));
  Connect(GAME_RUNNER_MENU_ID, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(BerryBotsApp::OnGameRunner));
  Connect(ERROR_CONSOLE_MENU_ID, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(BerryBotsApp::OnErrorConsole));
#ifndef __WXOSX__
  Connect(FILE_QUIT_MENU_ID, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(BerryBotsApp::OnQuit));
#endif

  Connect(CHANGE_BASE_DIR_MENU_ID, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(BerryBotsApp::onChangeBaseDir));
  Connect(BROWSE_SHIPS_MENU_ID, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(BerryBotsApp::onBrowseShips));
  Connect(BROWSE_STAGES_MENU_ID, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(BerryBotsApp::onBrowseStages));
  Connect(BROWSE_RUNNERS_MENU_ID, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(BerryBotsApp::onBrowseRunners));
  Connect(BROWSE_REPLAYS_MENU_ID, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(BerryBotsApp::onBrowseReplays));
  Connect(BROWSE_API_DOCS_MENU_ID, wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(BerryBotsApp::onBrowseApidocs));
  
  return true;
}

void BerryBotsApp::OnQuit(wxCommandEvent &event) {
  quit();
}

void BerryBotsApp::quit() {
  guiManager_->quit();
  ExitMainLoop();
  delete guiListener_;
  // TODO: delete guiManager_ without clobbering anyone else
}

void BerryBotsApp::OnNewMatch(wxCommandEvent &event) {
  guiManager_->showNewMatchDialog();
  guiManager_->newMatchInitialFocus();
}

void BerryBotsApp::OnPackageShip(wxCommandEvent &event) {
  guiManager_->showPackageShipDialog();
  guiManager_->packageShipInitialFocus();
}

void BerryBotsApp::OnPackageStage(wxCommandEvent &event) {
  guiManager_->showPackageStageDialog();
  guiManager_->packageStageInitialFocus();
}

void BerryBotsApp::OnGameRunner(wxCommandEvent &event) {
  guiManager_->showGameRunnerDialog();
  guiManager_->gameRunnerInitialFocus();
}

void BerryBotsApp::OnErrorConsole(wxCommandEvent &event) {
  guiManager_->showErrorConsole();
}

void BerryBotsApp::MacReopenApp() {
  // No-op - When SFML window is only one open and user switches to app via icon
  // in Mac OS X dock, this stops wxWidgets from opening one of its windows
  // unnecessarily.
}

void BerryBotsApp::onChangeBaseDir(wxCommandEvent &event) {
  chooseNewRootDir();
  guiManager_->reloadBaseDirs();
  guiManager_->loadStages();
  guiManager_->loadShips();
  guiManager_->loadRunners();
}

void BerryBotsApp::onBrowseStages(wxCommandEvent &event) {
  systemExecutor_->browseDirectory(getStagesDir().c_str());
}

void BerryBotsApp::onBrowseShips(wxCommandEvent &event) {
  systemExecutor_->browseDirectory(getShipsDir().c_str());
}

void BerryBotsApp::onBrowseRunners(wxCommandEvent &event) {
  systemExecutor_->browseDirectory(getRunnersDir().c_str());
}

void BerryBotsApp::onBrowseReplays(wxCommandEvent &event) {
  fileManager_->createDirectoryIfNecessary(getReplaysDir().c_str());
  systemExecutor_->browseDirectory(getReplaysDir().c_str());
}

void BerryBotsApp::onBrowseApidocs(wxCommandEvent &event) {
  systemExecutor_->openHtmlFile(getApidocPath().c_str());
}

std::string BerryBotsApp::dirName(const char *name) {
  std::stringstream dirName;
#ifdef __WXGTK__
  dirName << name << "/";
#else
  dirName << "\"" << name << "\"";
#endif
  return dirName.str();
}

AppGuiListener::AppGuiListener(BerryBotsApp *app) {
  app_ = app;
}

AppGuiListener::~AppGuiListener() {
  
}

void AppGuiListener::onAllWindowsClosed() {
  app_->quit();
}
