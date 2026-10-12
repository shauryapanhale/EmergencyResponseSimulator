# User guide

ERS is an educational simulation. Results are simulated estimates and must not be used for real emergencies.

## Starting

Run ers_gui.exe from the packaged folder (dist\ERS in the repository build, after running windeployqt). The window opens with the disclaimer, a 12-node fictional city and 6 vehicles (2 ambulances, 2 fire trucks, 2 police).

## Controls

| Control | What it does |
|---|---|
| Start / Pause / Resume / Step | Run the clock, stop it, continue, or advance one minute |
| Speed 1x, 2x, 5x, 10x | Real time per simulated minute: 500, 250, 100 or 50 ms |
| Reset | Clean city and fresh run; asks before discarding a run with incidents |
| Report incident | Type, location (preselected from the node you clicked), priority, description |
| Traffic / roads | Choose a road, set its traffic factor (0.5 to 5.0) or block it |
| Resolve selected | Close the selected incident once it is on scene and all vehicles have arrived |
| Change priority | Set Critical, High, Medium or Low for the selected incident |
| Statistics | Counts and times, including incidents resolved with an unmet resource |
| Scenario picker with Play scenario / Run instantly | Step a prepared scenario, or run it at once (incidents stay open because resolution is manual) |
| Save run / Saved runs | Store the current run and browse stored runs with their event logs |

## Map

Mouse wheel zooms toward the cursor, dragging pans, double-click zooms in, and the + / - / Fit buttons zoom or restore the whole city. Clicking a node selects it. Roads are gray (normal), yellow (traffic above 1.0), dark orange (above 1.5) and red dashed (blocked). Stations are squares, landmarks diamonds, intersections circles; incident rings are red; vehicles are boxes labelled A, F or P with their id.

## Keyboard shortcuts

F5 Start, F6 Pause, F7 Step, Ctrl+R Report incident, Ctrl+S Save run.

## How dispatch works

The nearest suitable available vehicle by estimated time is chosen. Estimated time uses the route length in minutes scaled by the vehicle speed (faster vehicles arrive sooner). The details pane shows the reason. If no suitable vehicle is free, the incident stays Pending and is retried every minute, Critical incidents first.

## Prepared scenarios

normal-traffic (60 min), rush-hour (90 min, heavy traffic), blocked-road (60 min, roads 9 and 18 closed, road 9 reopens at minute 30) and simultaneous-incidents (90 min, six incidents at minute 0).

## Where saved runs are stored

In ers_gui.db inside your user data folder: %APPDATA%\ERS\EmergencyResponseSimulator on Windows. Deleting the file removes all saved runs.

## Known limits

Vehicles already travelling keep their route if a road changes later. The selected dispatch route is not highlighted on the map.