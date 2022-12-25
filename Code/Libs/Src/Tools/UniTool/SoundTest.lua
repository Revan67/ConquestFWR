-- SoundTest.lua: A sound streaming testbed.
-- Pre-processor parameters
$debug

print ("Starting the sound tester...");

-- Music table
music = {};
musicDir = "d:\\mp3";

-- Utility functions
function AddMusic ()
	-- Ask for the new file to add to the list.
	local name = FileLoadDialog
	(
		"Play A File",
		"Media Files\000*.mp3;*.wav;*.avi;*.asf\000\000");
	if name ~= nil then
		musicList:add_string (name);
	end
end

function PlayRelativeMusic(filename)
	if music[filename] ~= nil then
		-- Stop the currently playing audio.
		StopStream(music[filename]);
		music[filename] = nil;
	end

	music[filename] = StartRelativeStream (filename, musicDir, 1);
end

function PlayMusic (filename)
	if music[filename] ~= nil then
		-- Stop the currently playing audio.
		StopStream(music[filename]);
		music[filename] = nil;
	end

	music[filename] = StartStream (filename, 1);
end

function StopMusic	(filename)
	-- If a current music exists, stop it.
	if music[filename] ~= nil then
		-- Stop the currently playing audio.
		StopStream(music[filename]);
		music[filename] = nil;
	end
end

-- Add some menus to the main window
AddMenuItem (MainWindow, "&Add Music File", FileMenu, 0, AddMusic);

-- Set up the main display window
musicListLabel = NewLabel (MainWindow, "Stopped List:", 0, 5, 300);
musicList = NewListBox (MainWindow, 0, musicListLabel:get_y() + musicListLabel:get_height(), 300, 300, "s");
musicList.OnDoubleClick = function (w, e)
	local name = w:get_item_string(w:get_select(0));
	musicList:del_named_string (name);
	musicPlayList:add_string (name);
	PlayMusic (name);
end

musicPlayListLabel = NewLabel (MainWindow, "Playing List:", 310, 5, 300);
musicPlayList = NewListBox (MainWindow, 310, musicPlayListLabel:get_y() + musicPlayListLabel:get_height(), 300, 300, "s");
musicPlayList.OnDoubleClick = function (w, e)
	local name = w:get_item_string(w:get_select(0));
	musicPlayList:del_named_string (name);
	musicList:add_string (name);
	StopMusic (name);
end
musicPlayList.OnChange = function (w, e)
	local name = w:get_item_string(w:get_select(0));
	if name then
		if (music[name]) then
			volumeScroll:set_scroll_pos (GetStreamVolume(music[name]));
		end
	end
end

volumeLabel = NewLabel
(
	MainWindow,
	"Current Volume",
	musicList:get_x(), musicList:get_y() + musicList:get_height() + 5
);

volumeScroll = NewScroll
(
	MainWindow,
	volumeLabel:get_x(), volumeLabel:get_y() + volumeLabel:get_height() + 5,
	300, 25,
	"h"
);

volumeScroll:set_scroll_range (-10000, 0);
volumeScroll:set_scroll_pos (0);
volumeScroll.OnChange = function (w, e)
	local pos = w:get_scroll_pos();
	local name = musicPlayList:get_item_string(musicPlayList:get_select(0));
	if name then
		if music[name] then 
			SetStreamVolume(music[name], pos);
		end
	end
end
-- We are done

-- Do a test of relative music playing.
--PlayRelativeMusic ("ooo.mp3");

print ("Sound test started.");
