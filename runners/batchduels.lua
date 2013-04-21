-- A Game Runner for batch duels. Takes as input a stage, a challenger, one or
-- more reference ships for the challenger to play against, and a number of
-- seasons. Runs <seasons> 1v1 matches of the challenger vs each reference ship
-- on the chosen stage and prints the results.

SETTINGS_FILE = "settings/batchduels.properties"

scoreKeys = { }
totalScores = { }
shipKeys = { } 
scoresByShip = { }
numResults = 0
challenger = nil

function run(runner, form, files, network)
  form:addStageSelect("Stage")
  form:addSingleShipSelect("Challenger")
  form:addMultiShipSelect("Reference Ships")
  form:addIntegerText("Seasons")
  form:addIntegerText("Threads")
  defaultSettings(form, files)

  if (form:ok()) then
    local stage = form:get("Stage")
    challenger = form:get("Challenger")
    local referenceShips = form:get("Reference Ships")
    local seasons = form:get("Seasons")
    local threadCount = form:get("Threads")
    saveSettings(files, stage, challenger, referenceShips, seasons, threadCount)

    print("Seasons: " .. seasons)
    print("Challenger: " .. challenger)
    print("Stage: " .. stage)
    for i, referenceShip in pairs(referenceShips) do
      print("Reference ship: " .. referenceShip)
    end
    print()
    runner:setThreadCount(threadCount)

    for i = 1, seasons do
      for j, shipName in ipairs(referenceShips) do
        runner:queueMatch(stage, {challenger, shipName})
      end
    end

    while (not runner:empty()) do
      processNextResult(runner)
    end

    print()
    print("---------------------------------------------------------------------")
    print("Overall results")
    print("---------------------------------------------------------------------")
    print("vs all ships:")
    printScores(totalScores)
    for i, ship in ipairs(shipKeys) do
      print("---------------------------------------------------------------------")
      print("vs " .. ship)
      printScores(scoresByShip[ship])
    end
  else
    -- user canceled, do nothing
  end
end

function defaultSettings(form, files)
  if (files:exists(SETTINGS_FILE)) then
    local settingsFile = files:read(SETTINGS_FILE)
    local stage, challenger, seasons, threads, t
    t, t, stage = string.find(settingsFile, "Stage=([^\n]*)\n")
    t, t, challenger = string.find(settingsFile, "Challenger=([^\n]*)\n")
    t, t, seasons = string.find(settingsFile, "Seasons=([^\n]*)\n")
    t, t, threads = string.find(settingsFile, "Threads=([^\n]*)\n")

    if (stage ~= nil) then form:default("Stage", stage) end
    if (challenger ~= nil) then form:default("Challenger", challenger) end
    if (seasons ~= nil) then form:default("Seasons", seasons) end
    if (threads ~= nil) then form:default("Threads", threads) end
    for ship in string.gmatch(settingsFile, "referenceShip=([^\n]*)\n") do
      form:default("Reference Ships", ship)
    end
  else
    form:default("Stage", "sample/battle1.lua")
    form:default("Seasons", 10) 
    form:default("Threads", 2) 
  end
end

function saveSettings(files, stage, challenger, referenceShips, numSeasons,
                      threadCount)
  local settings = "Stage=" .. stage .. "\n"
      .. "Challenger=" .. challenger .. "\n"
  for i, ship in ipairs(referenceShips) do
    settings = settings .. "referenceShip=" .. ship .. "\n"
  end
  settings = settings .. "Seasons=" .. numSeasons .. "\n"
      .. "Threads=" .. threadCount .. "\n"
  files:write(SETTINGS_FILE, settings)
end

function processNextResult(runner)
  local result = runner:nextResult()
  local teams = result.teams
  print("---------------------------------------------------------------------")
  if (result.errored) then
    print("    Error: " .. result.errorMessage)
  else
    print(teams[1].name .. " vs " .. teams[2].name)
    if (result.winner == nil) then
      print("    Tie.")
    else
      print("    " .. result.winner .. " wins!")
    end
    print("------------------------------")
    local referenceShip = teams[2].name
    local sortedTeams = getSortedTeams(teams)
    for i, team in ipairs(sortedTeams) do
      if (i > 1) then
        print("--------")
      end
      print("    " .. team.name .. ":")
      print("        Rank: " .. team.rank)
      print("        Score: " .. round(team.score, 2))
      if (team.stats ~= nil) then
        print("        Stats:")
        for key, value in pairs(team.stats) do
          print("            " .. key .. ": " .. round(value, 2))
        end
      end
      if (team.name == challenger) then
        saveShipScore(referenceShip, "Rank", team.rank)
        saveShipScore(referenceShip, "Score", team.score)
        if (team.stats ~= nil) then
          for key, value in pairs(team.stats) do
            saveShipScore(referenceShip, key, value)
          end
        end
        numResults = numResults + 1
      end
    end
  end
end

function printScores(scores)
  for i, key in ipairs(scoreKeys) do
    print("    " .. key .. ": " .. round(scores[key] / numResults, 2))
  end
end

function saveShipScore(ship, key, value)
  if (scoresByShip[ship] == nil) then
    scoresByShip[ship] = { }
  end
  saveShipKey(ship)
  saveScoreKey(key)
  saveScore(scoresByShip[ship], key, value)
  saveTotalScore(key, value)
end

function saveTotalScore(key, value)
  saveScore(totalScores, key, value)
end

function saveScore(scores, key, value)
  if (scores[key] == nil) then
    scores[key] = value
  else
    scores[key] = scores[key] + value
  end
end

function saveScoreKey(newKey)
  saveKey(scoreKeys, newKey)
end

function saveShipKey(newKey)
  saveKey(shipKeys, newKey)
end

function saveKey(keys, newKey)
  for i, key in pairs(keys) do
    if (key == newKey) then
      return
    end
  end
  table.insert(keys, newKey)
end

function getSortedTeams(teams)
  local sortedTeams = { }
  for i, team in pairs(teams) do
    table.insert(sortedTeams, team)
  end
  table.sort(sortedTeams, teamSorter)
  return sortedTeams
end

function teamSorter(team1, team2)
  if (team1.rank < team2.rank or (team1.rank > 0 and team2.rank == 0)) then
    return true
  end
  return false
end

function round(d, x)
  local powerTen = 1
  for i = 1, x do
    powerTen = powerTen * 10
  end
  return math.floor((d * powerTen) + .5) / powerTen
end
