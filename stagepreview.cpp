/*
  Copyright (C) 2013 - Voidious

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

#include <string.h>
#include <sstream>
#include <algorithm>
#include <wx/wx.h>
#include <wx/webview.h>
#include "bbconst.h"
#include "bbwx.h"
#include "ResourcePath.hpp"
#include "basedir.h"
#include "filemanager.h"
#include "sysexec.h"
#include "menubarmaker.h"
#include "bbengine.h"
#include "stagepreview.h"

StagePreview::StagePreview(const char *stagesBaseDir,
                           MenuBarMaker *menuBarMaker)
    : wxFrame(NULL, wxID_ANY, "Preview", wxDefaultPosition, wxDefaultSize,
              wxDEFAULT_FRAME_STYLE & ~ (wxRESIZE_BORDER | wxMAXIMIZE_BOX)) {
  menuBarMaker_ = menuBarMaker;
  menusInitialized_ = false;
  fileManager_ = new FileManager();
  listener_ = 0;
  systemExecutor_ = new SystemExecutor();
  stagesBaseDir_ = new char[strlen(stagesBaseDir) + 1];
  strcpy(stagesBaseDir_, stagesBaseDir);
  stageName_ = 0;

#ifdef __WINDOWS__
  SetIcon(wxIcon(BERRYBOTS_ICO, wxBITMAP_TYPE_ICO));
  
  // The 8-9 point default font size in Windows is much smaller than Mac/Linux.
  wxFont windowFont = GetFont();
  if (windowFont.GetPointSize() <= 9) {
    SetFont(windowFont.Larger());
  }
#elif __WXGTK__
  SetIcon(wxIcon(BBICON_128, wxBITMAP_TYPE_PNG));
#endif

  mainPanel_ = new wxPanel(this);
  infoSizer_ = new wxStaticBoxSizer(wxVERTICAL, mainPanel_);
  descSizer_ = new wxStaticBoxSizer(wxVERTICAL, mainPanel_);

#ifndef __WINDOWS__
  // On Windows, wxWebView is powered by IE and balks at showing Javascript
  // because it's "active content". For now just show info/description.
  webView_ = wxWebView::New(mainPanel_, wxID_ANY);
  webView_->EnableContextMenu(false);
  webView_->EnableHistory(false);
#endif

  wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
  mainSizer->Add(mainPanel_, 0, wxEXPAND);
  wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);

#ifndef __WINDOWS__
  topSizer->Add(webView_, 0, wxEXPAND);
  topSizer->AddSpacer(8);
#endif

  wxBoxSizer *rowSizer = new wxBoxSizer(wxHORIZONTAL);
  rowSizer->Add(infoSizer_, 0, wxEXPAND);
  rowSizer->AddSpacer(4);
  rowSizer->Add(descSizer_, 1, wxEXPAND);
  topSizer->Add(rowSizer, 0, wxEXPAND);

  wxBoxSizer *borderSizer = new wxBoxSizer(wxVERTICAL);
  borderSizer->Add(topSizer, 0, wxALL | wxEXPAND, 12);
  mainPanel_->SetSizerAndFit(borderSizer);
  SetSizerAndFit(mainSizer);

  Connect(this->GetId(), wxEVT_ACTIVATE,
          wxActivateEventHandler(StagePreview::onActivate));
  Connect(this->GetId(), wxEVT_CLOSE_WINDOW,
          wxCommandEventHandler(StagePreview::onClose));
#ifndef __WINDOWS__
  Connect(webView_->GetId(), wxEVT_WEBVIEW_LOADED,
          wxWebViewEventHandler(StagePreview::onWebViewLoaded));
  Connect(webView_->GetId(), wxEVT_WEBVIEW_ERROR,
          wxWebViewEventHandler(StagePreview::onWebViewError));
#endif

  eventFilter_ = new PreviewEventFilter(this);
  this->GetEventHandler()->AddFilter(eventFilter_);
}

StagePreview::~StagePreview() {
  this->GetEventHandler()->RemoveFilter(eventFilter_);
  if (listener_ != 0) {
    delete listener_;
  }
  delete eventFilter_;
  delete fileManager_;
  delete stagesBaseDir_;
  if (stageName_ != 0) {
    delete stageName_;
  }
  delete systemExecutor_;
}

void StagePreview::onActivate(wxActivateEvent &event) {
#ifndef __WINDOWS__
  if (!menusInitialized_) {
    this->SetMenuBar(menuBarMaker_->getNewMenuBar());
    menusInitialized_ = true;
  }
#endif
  Fit();
}

void StagePreview::onClose(wxCommandEvent &event) {
  if (listener_ != 0) {
    listener_->onClose();
  }
  Hide();
}

void StagePreview::onWebViewLoaded(wxWebViewEvent &event) {
  if (listener_ != 0 && stageName_ != 0) {
    listener_->onLoaded(stageName_);
  }

#ifdef __WXOSX__
  // On Mac/Cocoa, we hide the window while loading the preview and show it in
  // response to the loaded event.
  Show();
  Raise();
#endif
}

void StagePreview::onWebViewError(wxWebViewEvent &event) {
  if (listener_ != 0 && stageName_ != 0) {
    listener_->onLoaded(stageName_);
  }

  wxMessageDialog errorMessage(NULL, wxString::Format(wxT(
      "Error loading stage preview graphics from URL: %s\n\n%s (%i)"),
          event.GetURL(), event.GetString(), event.GetInt()),
          "Preview failure", wxOK | wxICON_EXCLAMATION);
  errorMessage.ShowModal();

#ifdef __WXOSX__
  // On Mac/Cocoa, we hide the window while loading the preview and show it in
  // response to the loaded event.
  Show();
  Raise();
#endif
}

void StagePreview::onVisualPreview(wxCommandEvent &event) {
  systemExecutor_->openHtmlFile(lastPreviewUrl_.substr(7).c_str()); // 7 = "file://"
}

void StagePreview::onUp() {
  if (listener_ != 0) {
    listener_->onUp();
  }
}

void StagePreview::onDown() {
  if (listener_ != 0) {
    listener_->onDown();
  }
}

void StagePreview::setListener(StagePreviewListener *listener) {
  if (listener_ != 0) {
    delete listener_;
  }
  listener_ = listener;
}

void StagePreview::showPreview(const char *stageName, int x, int y) {
#ifdef __WXOSX__
  // On Mac/Cocoa, we hide the window while loading the preview and show it in
  // response to the loaded event.
  Hide();
#endif

  infoSizer_->Clear(true);
  descSizer_->Clear(true);

  if (stageName_ != 0) {
    delete stageName_;
  }
  stageName_ = new char[strlen(stageName) + 1];
  strcpy(stageName_, stageName);

  SetPosition(wxPoint(x, y));
  BerryBotsEngine *engine =
      new BerryBotsEngine(0, fileManager_, resourcePath().c_str());
  Stage *stage = engine->getStage();

  try {
    engine->initStage(stagesBaseDir_, stageName, getCacheDir().c_str());
  } catch (EngineException *e) {
    wxMessageDialog errorMessage(NULL, e->what(), "Preview failure",
                                 wxOK | wxICON_EXCLAMATION);
    errorMessage.ShowModal();
    delete engine;
    delete e;
    return;
  }
  SetTitle(wxString::Format(wxT("%s"), stage->getName()));

  double stageWidth = stage->getWidth() + (STAGE_MARGIN * 2);
  double stageHeight = stage->getHeight() + (STAGE_MARGIN * 2);
  double previewScale = std::min(1.0,
      std::min(((double) MAX_PREVIEW_WIDTH) / stageWidth,
               ((double) MAX_PREVIEW_HEIGHT) / stageHeight));
  std::string previewUrl = savePreviewReplay(engine);
  lastPreviewUrl_ = previewUrl;
#ifndef __WINDOWS__
  webView_->SetSizeHints(previewScale * stageWidth, previewScale * stageHeight);
  webView_->LoadURL(previewUrl);
#endif

#ifdef __WXOSX__
  int padding = 4;
#else
  int padding = 8;
#endif
  wxSizer *infoGrid = new wxFlexGridSizer(2, 0, padding);
  addInfo(infoGrid, "Name:", stage->getName());
  addInfo(infoGrid, "Size:",
      wxString::Format(wxT("%i x %i"), stage->getWidth(), stage->getHeight()));
  if (engine->getTeamSize() > 1) {
    addInfo(infoGrid, "Team size:", engine->getTeamSize());
  }
  addInfo(infoGrid, "Walls:", (stage->getWallCount() - 4));
  addInfo(infoGrid, "Zones:", stage->getZoneCount());
  addInfo(infoGrid, "Starts:", stage->getStartCount());
  int numStageShips = stage->getStageShipCount();
  if (numStageShips > 0) {
    char **stageShips = stage->getStageShips();
    for (int x = 0; x < numStageShips; x++) {
      const char *shipName = stageShips[x];
      if (shipName != 0) {
        int count = 1;
        for (int y = x + 1; y < numStageShips; y++) {
          const char *shipName2 = stageShips[y];
          if (shipName2 != 0 && strcmp(shipName, shipName2) == 0) {
            count++;
            stageShips[y] = 0;
          }
        }
        wxString wxShipName = (count == 1) ? wxString(stageShips[x])
            : wxString::Format(wxT("%s x%i"), shipName, count);
        addInfo(infoGrid, (x == 0 ? "Ships:" : ""), wxShipName);
      }
    }
  }
#ifdef __WINDOWS__
  wxButton *viewStageButton = new wxButton(mainPanel_, wxID_ANY, "&View Stage");
  Connect(viewStageButton->GetId(), wxEVT_COMMAND_BUTTON_CLICKED,
          wxCommandEventHandler(StagePreview::onVisualPreview));
  infoGrid->AddSpacer(8);
  infoGrid->AddSpacer(8);
  infoGrid->AddSpacer(0);
  infoGrid->Add(viewStageButton, 0, wxALIGN_RIGHT);
#endif
  infoSizer_->Add(infoGrid);

  char *description = fileManager_->getStageDescription(
      stagesBaseDir_, stageName, getCacheDir().c_str());
  if (description == 0) {
    std::string descstr("<No description>");
    description = new char[descstr.length() + 1];
    strcpy(description, descstr.c_str());
  }
  wxStaticText *descCtrl = new wxStaticText(mainPanel_, wxID_ANY, description);
  descSizer_->Add(descCtrl);
  delete description;

  mainPanel_->GetSizer()->SetSizeHints(mainPanel_);
  mainPanel_->Layout();
  mainPanel_->SetFocus();
  delete engine;

#ifndef __WXOSX__
  Show();
#endif
}

void StagePreview::addInfo(wxSizer *sizer, const char *name,
                           const char *value) {
  if (name != 0) {
    sizer->Add(new wxStaticText(mainPanel_, wxID_ANY, name));
  }
  sizer->Add(new wxStaticText(mainPanel_, wxID_ANY, value));
}

void StagePreview::addInfo(wxSizer *sizer, const char *name, int i) {
  if (i > 0) {
    sizer->Add(new wxStaticText(mainPanel_, wxID_ANY, name));
    sizer->Add(new wxStaticText(mainPanel_, wxID_ANY,
                                wxString::Format(wxT("%i"), i)));
  }
}

std::string StagePreview::savePreviewReplay(BerryBotsEngine *engine) {
  Stage *stage = engine->getStage();
  ReplayBuilder *previewReplay = engine->getReplayBuilder();
  previewReplay->initShips(1, 1);

  Team *previewTeam = new Team;
  strcpy(previewTeam->name, " ");
  previewTeam->index = 0;
  previewReplay->addTeamProperties(previewTeam);

  Ship *previewShip = new Ship;
  ShipProperties *properties = new ShipProperties;
  properties->shipR = properties->shipG = properties->shipB = 255;
  properties->laserR = properties->laserB = 0;
  properties->laserG = 255;
  properties->thrusterR = 255;
  properties->thrusterG = properties->thrusterB = 0;
  strcpy(properties->name, " ");
  previewShip->properties = properties;
  previewShip->index = previewShip->teamIndex = 0;
  previewReplay->addShipProperties(previewShip);

  previewShip->thrusterAngle = previewShip->thrusterForce = 0;
  Point2D *start = stage->getStart();
  previewShip->x = start->getX();
  previewShip->y = start->getY();
  previewShip->alive = true;
  previewShip->showName = previewShip->energyEnabled = false;
  Ship **previewShips = new Ship*[1];
  previewShips[0] = previewShip;
  previewReplay->addShipStates(previewShips, 1);

  delete previewShips;
  delete properties;
  delete previewShip;
  delete previewTeam;
  delete start;

  std::stringstream filenameStream;
  filenameStream << (rand() % 10000000) << ".html";
  previewReplay->setExtraJavascript("BerryBots.interactive = false;");
  previewReplay->saveReplay(getTmpDir().c_str(),
                            filenameStream.str().c_str());

  std::string previewUrl("file://");
  char *previewPath = fileManager_->getFilePath(getTmpDir().c_str(),
                                                filenameStream.str().c_str());
  previewUrl.append(previewPath);
  delete previewPath;
  delete previewReplay;

  return previewUrl;
}

PreviewEventFilter::PreviewEventFilter(StagePreview *stagePreview) {
  stagePreview_ = stagePreview;
}

int PreviewEventFilter::FilterEvent(wxEvent& event) {
  bool modifierDown = false;
  wxKeyEvent *keyEvent = ((wxKeyEvent*) &event);
#if defined(__WXOSX__)
  modifierDown = keyEvent->ControlDown();
#elif defined(__WINDOWS__)
  modifierDown = keyEvent->AltDown();
#endif

  const wxEventType type = event.GetEventType();
  if (type == wxEVT_KEY_DOWN && stagePreview_->IsActive()) {
    int keyCode = keyEvent->GetKeyCode();
    if (keyCode == WXK_ESCAPE || keyCode == WXK_SPACE
        || (keyEvent->GetUnicodeKey() == 'W' && keyEvent->ControlDown())) {
      stagePreview_->Close();
      return Event_Processed;
#ifdef __WXOSX__
    // So far this only works smoothly on Mac. I'd rather disable it on other
    // platforms until it's up to par.
    } else if (keyCode == WXK_UP) {
      stagePreview_->onUp();
      return Event_Processed;
    } else if (keyCode == WXK_DOWN) {
      stagePreview_->onDown();
      return Event_Processed;
#endif
    }
  }

  return Event_Skip;
}
