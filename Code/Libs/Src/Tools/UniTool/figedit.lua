-- FigEdit.lua: A Figure Editor implemented using UniTool
-- Pre-processor parameters
$debug

print ("Starting the figure editor...");

oldPos = Vector(0,0,0);

-- LUA Code Execution Window.
CodeWin = NewTopLevel("LUA Code Executor", 0, 0, 600, 200);
CodeWin:set_visible(nil);
ExecBtn = NewButton
(
	CodeWin,
	"Run Contents",
	0, 0
);

ExecBtn.OnPress = function (widget, eventName)
	dostring (tt:get_text(), "Run");
end

tt = NewText (CodeWin, "--Insert LUA code here", 0, ExecBtn:get_height(), CodeWin:get_width(), CodeWin:get_height() - ExecBtn:get_height());

CodeWin.OnSize = function (widget, eventName)
	tt:set_size (CodeWin:get_width(), CodeWin:get_height() - ExecBtn:get_height());
end

tt.OnChange = function (widget, eventName)
	if strlen(widget:get_text()) > 128 then
		print ("Text too long!");
	end
end

function ToggleCodeWindow()
	if CodeWin:get_visible() then
		CodeWin:set_visible (nil);
	else
		CodeWin:set_visible (1);
	end
end

-- Utility functions
function CreateTextWindow (title, width, height)
	local dumpwin;
	dumpwin = NewTopLevel (title, 0, 0, width, height);
	dumpwin.text_widget = NewText (dumpwin, "", 0, 0, dumpwin:get_width(), dumpwin:get_height());
	dumpwin.OnSize = function (widget, eventName)
		widget.text_widget:set_size (widget:get_width(), widget:get_height());
	end
	dumpwin.append_str = function (this, str)
		local old_text = this.text_widget:get_text();
		if old_text == nil then
			this.text_widget:set_text (str);
		else
			this.text_widget:set_text(old_text .. str);
		end
	end
	dumpwin.print = function (this, str)
		--this:append_str (str .. "\r\n");
		this.text_widget:set_text (str .. "\r\n");
	end
	return dumpwin;
end

function SetupRenderer (width, height, multiple)
	-- Create the render window and the default camera, both global variables
	CreateRender ("Test Render Window", 0, 0, width, height);
        RenderWindow:set_position (660, 0);
	camera = NewCamera (0,       0,        width,   height);
	camera.pitch = 0;
	camera.roll = 0;
	camera.yaw = 0;

	-- Set the initial position for the default camera.
	local initPos = Vector(0,0,20);
	camera:set_pos (initPos);

	-- Set the initial rendering conditions
	RenderWindow:set_ambient(255);
	RenderWindow:set_clear_color (0);

	-- Set up some controls on the render window.
	RenderWindow.OnSize = function (this, eventName)
		local w, h;
		w = this:get_width();
		h = this:get_height();

		camera:set_viewport (0, 0, w, h);
		this.zoomscroll:set_size
		(
			this:get_width() - this.zoomscroll:get_x(),
			this.zoomscroll:get_height()
		);
	end

	RenderWindow.animate = NewButton
	(
		RenderWindow,
		"Animate Camera",
		10, 0
	);

	RenderWindow.animate.OnPress = function (this, eventName)
		local zpos = 0.5;
		local pos = Vector(0,0,zpos);
		while zpos < 100 do
			camera:set_pos (pos);
			zpos = zpos + 0.5
			pos.z = zpos;
		end
	end

	RenderWindow.quit = NewButton
	(
		RenderWindow,
		"Quit",
		RenderWindow.animate:get_x() + RenderWindow.animate:get_width() + 10, 0
	);

	RenderWindow.quit.OnPress = function (this, eventName)
		QuitUnitool();
	end

	RenderWindow.zoomscroll = NewScroll 
	(
		RenderWindow, 
		RenderWindow.quit:get_x() + RenderWindow.quit:get_width() + 5, 0,
		RenderWindow:get_width() - (RenderWindow.quit:get_x() + RenderWindow.quit:get_width() + 5), 20,
		"h"
	);

	RenderWindow.zoomscroll.OnChange = function (widget, eventName)
		pos = widget:get_scroll_pos();
		local v = camera:get_pos();
		v.z = pos;
		camera:set_pos(v);
	end

	RenderWindow.OnUpdate = function (dt, frameTime)
		-- print ("Delta Time == ", dt);
	end

	RenderWindow.objmoveFunction = function (rw, eName, fwKeys, xPos, yPos)
		-- print ("MMove: ", fwKeys, xPos, yPos);
		-- Move the current object according to the mouse movement if the shift key is down
		if strfind (fwKeys, "s", 1, 1) then
			local pos = object:get_pos();
			pos.z = pos.z + (rw.lastY - yPos);
			object:set_pos(pos);
		elseif strfind (fwKeys, "c", 1, 1) then
			local pos = object:get_pos();
			dumpWin:print (format ("Pos at start=(%f,%f,%f)", pos.x, pos.y, pos.z));
			if (pos.x ~= oldPos.x or pos.y ~= oldPos.y or pos.z ~= oldPos.z) then
				dumpWin:print ("Position changed!");
			end
			object.pitch = object.pitch + yPos - rw.lastY;
			object.yaw = object.yaw + xPos - rw.lastX;
			--local xform = Xform (yPos - rw.startY, 0, xPos - rw.startX);
			local xform = Xform (object.pitch, object.roll, object.yaw);
			object:set_transform(xform);
			object:set_pos (pos);
			dumpWin:print
			(
				format
				(
					"Object pos: (%f, %f, %f)\r\nYaw=%f\r\nPitch=%f\r\nRoll=%f\r\n",
					pos.x, pos.y, pos.z,
					object.yaw, object.pitch, object.roll
				)
			);
			oldPos = pos;
		end
		rw.lastX = xPos;
		rw.lastY = yPos;
	end

	RenderWindow.cammoveFunction = function (rw, eName, fwKeys, xPos, yPos)
		-- print ("MMove: ", fwKeys, xPos, yPos);
		-- Move the current camera according to the mouse movement if the shift key is down
		if strfind (fwKeys, "s", 1, 1) then
			local pos = camera:get_pos();
			pos.z = pos.z + (rw.lastY - yPos);
			camera:set_pos(pos);
		elseif strfind (fwKeys, "c", 1, 1) then
			camera.pitch = camera.pitch + yPos - rw.lastY;
			camera.yaw = camera.yaw + xPos - rw.lastX;
			--local xform = Xform (yPos - rw.startY, 0, xPos - rw.startX);
			local xform = Xform (camera.pitch, camera.roll, camera.yaw);
			camera:set_transform(xform);
			camera:set_pos (pos);
		end
		rw.lastX = xPos;
		rw.lastY = yPos;
	end

	RenderWindow.OnMouseDown = function (rw, eName, fwKeys, xPos, yPos)
		--print ("Mouse down");
		-- Enable mouse movement capturing
--		rw.OnMouseMove = rw.objmoveFunction;
		rw.OnMouseMove = rw.cammoveFunction;
		rw.startX = xPos;
		rw.startY = yPos;
		rw.lastX = xPos;
		rw.lastY = yPos;
	end

	RenderWindow.OnMouseUp = function (rw, eName, fwKeys, xPos, yPos)
		--print ("Mouse up");
		-- If there are no mouse buttons down, disable the movement capturing
		if ((not strfind (fwKeys, "l", 1, 1)) and
			(not strfind (fwKeys, "r", 1, 1)) and
			(not strfind (fwKeys, "m", 1, 1)))
		then
			rw.OnMouseMove = nil;
		end
	end
end

function LoadOneObject (name)
	if name ~= nil then
		object = NewObject (name);
		objectName:set_text ("Current Object: "..name);

		-- Set the object orientation values to their default.
		object.pitch = 0;
		object.roll = 0;
		object.yaw = 0;

		local anims = object.anim;
		if anims ~= nil then
			objectAnims:reset();
			local i, script;
			i, script = next (anims, nil);
			while i ~= nil do
				objectAnims:add_string (script.name);
				i, script = next(anims, i);
			end
		end
	end
end

function LoadObject	()
	-- If a current object exists, destroy it.
	if object ~= nil then
		DestroyObject(object);
		object = nil;
	end

	-- Ask for the new object to load
	local name = FileLoadDialog("Load An Object", "Object Files\000*.3db;*.cmp\000\000");
	LoadOneObject (name);
end

function PlayMusic	()
	-- If a current music exists, stop it.
	if music ~= nil then
		StopStream(music);
		music = nil;
	end

	-- Ask for the new file to play
	local name = FileLoadDialog("Play A File", "MP3 Files\000*.mp3\000\000");
	music = StartStream (name);
end

function StopMusic	()
	-- If a current music exists, stop it.
	if music ~= nil then
		StopStream(music);
		music = nil;
	end
end

function ResetAnimation (animName, anim)
	anim:set_time(0.0);
	anim:start();
	anim.stop();
end

function ResetAnimations ()
	-- Only do this if an object exists
	if object ~= nil then
		foreach (object.anim, ResetAnimation);
	end
end

function DumpObjectData ()
	-- Dump some information about the object here.
	local pos = object:get_pos();
	local str = 
		format
		(
			"Object pos: (%f, %f, %f)\r\nYaw=%f\r\nPitch=%f\r\nRoll=%f\r\n",
			pos.x, pos.y, pos.z,
			object.yaw, object.pitch, object.roll
		)

	dumpWin:append_str(str);
end

videoWidget = nil;

function PlayMovie ()
	-- Ask for the filename of the movie to play, create a movie window, and play the movie.
	local name = FileLoadDialog("Play A Movie", "Movie Files\000*.avi\000\000");
	if name ~= nil then
		CreateVideo("Video:" .. name, 0, 0, 640, 480);
		 
		if VideoWindow ~= nil then
			VideoWindow:play_movie (name);
		end
	end
end

function StartMovie ()
	-- Tell the video window to start playing a movie
	local name = FileLoadDialog("Play A Movie", "Movie Files\000*.avi\000\000");
	if name ~= nil then
		CreateVideo("Video:" .. name, 0, 0, 640, 480);

		if VideoWindow ~= nil then
			VideoWindow:start_movie (name);
		end
	end
end

function StopMovie ()
	-- Tell the video window to stop playing the movie
	if VideoWindow ~= nil then
		VideoWindow:stop_movie ();
	end
end

-- Add some menus to the main window
AddMenuItem (MainWindow, "&Load Object", FileMenu, 0, LoadObject);
AddMenuItem (MainWindow, "&Play Movie",  FileMenu, 1, PlayMovie);
AddMenuItem (MainWindow, "&Start Movie",  FileMenu, 2, StartMovie);
AddMenuItem (MainWindow, "St&op Movie",  FileMenu, 3, StopMovie);
AddMenuItem (MainWindow, "Play &MP3",  FileMenu, 4, PlayMusic);
AddMenuItem (MainWindow, "S&top MP3",  FileMenu, 5, StopMusic);
ObjectMenu = AddMenu (MainWindow, AddMenuItem (MainWindow, "&Object", MainMenu, -1, nil));
AddMenuItem (MainWindow, "&Reset Animations", ObjectMenu, 1, ResetAnimations);
AddMenuItem (MainWindow, "&Get Info", ObjectMenu, 2, DumpObjectData);
MiscMenu = AddMenu (MainWindow, AddMenuItem (MainWindow, "&Misc", MainMenu, -1, nil));
AddMenuItem (MainWindow, "&Toggle Code Window", MiscMenu, 1, ToggleCodeWindow);

function TestProperties ()
	print ("Object Properties:\n");
	local propTable = object:get_properties();
	if propTable ~= nil then
		foreach
		(
			propTable,
			function (index, value)
				print (index, "=", value);
			end
		);
	else
		print ("The object has no properties.\n");
	end

	print ("Archetype Properties:\n");
	local archPropTable = object:get_arch_properties();
	if archPropTable ~= nil then
		foreach
		(
			archPropTable,
			function (index, value)
				print (index, "=", value);
			end
		);
	else
		print ("The object's archetype has no properties.\n");
	end
end

AddMenuItem (MainWindow, "&Test Properties", MiscMenu, 2, TestProperties);

overlay = nil;
function TestOverlays ()
	print ("Testing overlays.\n");
	local font = CreateFont ("Arial", 12);
	if overlay ~= nil then
		DestroyOverlay (overlay);
		overlay = nil;
	end
	overlay = NewImageOverlay ("test.bmp");
--	overlay = NewTextOverlay (font, "This is some text to test the overlay creation code.", 512, 512);
	DestroyFont (font);
end

psys = nil;
function TestParticles ()
	print ("Testing particles.\n");

	-- Destroy any existing object.
	if object ~= nil then
		DestroyObject(object);
		object = nil;
	end

	-- Create a new particle system
	-- Syntax: CreateParticleSystem (table emitterDesc, table behaviorDesc, table rendererDesc);
	psys = CreateParticleSystem
	(
		{type = NOZZLE, dir = Vector(0,1,0), angle = 45, rate = 60, minVel = 9.0, maxVel = 20.0},
		{type = BALLISTIC, gravity = Vector(0,-9.8,0), floorNormal = Vector(0,1,0), floorPoint = Vector(0,-5,0)},
		{type = BILLBOARD, texture = "partic3", minRad = 0.1, maxRad = 0.2},
--		{type = OBJECTPARTICLE, objFile = "camera.cmp"},
		256
	);
end

AddMenuItem (MainWindow, "Test &Overlays", MiscMenu, 3, TestOverlays);
AddMenuItem (MainWindow, "Test Mode &Switch", MiscMenu, 4, TestModeSwitch);
AddMenuItem (MainWindow, "Test &Particles", MiscMenu, 5, TestParticles);

-- Set up the rendering environment.
-- SetupRenderer(480, 360);
SetupRenderer(640, 480);

-- Set up the main display window
objectName = NewLabel (MainWindow, "Current Object: <None>", 0, 5, 640);
objAnimLabel = NewLabel (MainWindow, "Animations:", 0, objectName:get_y() + objectName:get_height() + 5, 300);
objectAnims = NewListBox (MainWindow, 0, objAnimLabel:get_y() + objAnimLabel:get_height(), 300, 300, "s");
objectAnims.OnDoubleClick = function (w, e)
	local name = w:get_item_string(w:get_select(0));
	object.anim[name]:set_time(0.0);
	object.anim[name]:start();
end
dumpWin = CreateTextWindow ("Debug Output", 640, 200);
dumpWin:set_position (0, 510);

ambLabel = NewLabel
(
	MainWindow,
	"Ambient Level",
	objAnimLabel:get_x() + objAnimLabel:get_width() + 5, objectName:get_y() + objectName:get_height() + 5
);

ambientScroll = NewScroll
(
	MainWindow,
	ambLabel:get_x(), ambLabel:get_y() + ambLabel:get_height() + 5,
	300, 25,
	"h"
);
ambientScroll:set_scroll_range (0, 255);
ambientScroll:set_scroll_pos (255);
ambientScroll.OnChange = function (w, e)
	local pos = w:get_scroll_pos();
	RenderWindow:set_ambient(pos);
end

clearLabel = NewLabel
(
	MainWindow,
	"Clear Color",
	objAnimLabel:get_x() + objAnimLabel:get_width() + 5, ambientScroll:get_y() + ambientScroll:get_height() + 5
);

clearScroll = NewScroll
(
	MainWindow,
	clearLabel:get_x(), clearLabel:get_y() + clearLabel:get_height() + 5,
	300, 25,
	"h"
);

clearScroll:set_scroll_range (0, 100);
clearScroll:set_scroll_pos (0);
clearScroll.OnChange = function (w, e)
	local pos = w:get_scroll_pos();
	RenderWindow:set_clear_color(pos/100);
end

timeLabel = NewLabel
(
	MainWindow,
	"Current Time",
	objAnimLabel:get_x() + objAnimLabel:get_width() + 5, clearScroll:get_y() + clearScroll:get_height() + 5
);

timeScroll = NewScroll
(
	MainWindow,
	timeLabel:get_x(), timeLabel:get_y() + timeLabel:get_height() + 5,
	300, 25,
	"h"
);

timeScroll:set_scroll_range (0, 100);
timeScroll:set_scroll_pos (0);
timeScroll.OnChange = function (w, e)
	local pos = w:get_scroll_pos();
	local name = objectAnims:get_item_string(objectAnims:get_select(0));
	if name then
		local script = object.anim[name];
		local duration = script:get_duration();
		script:set_time(duration * pos / 100.0);
		object.anim[name]:start();
		object.anim[name]:stop();
		RenderWindow:refresh();
	end
end

gammaLabel = NewLabel
(
	MainWindow,
	"Gamma Level",
	objAnimLabel:get_x() + objAnimLabel:get_width() + 5, timeScroll:get_y() + timeScroll:get_height() + 5
);

gammaScroll = NewScroll
(
	MainWindow,
	gammaLabel:get_x(), gammaLabel:get_y() + gammaLabel:get_height() + 5,
	300, 25,
	"h"
);

gammaScroll:set_scroll_range (-10000, 10000);
gammaScroll:set_scroll_pos (0);
gammaScroll.OnChange = function (w, e)
	local gamma = w:get_scroll_pos() / 10000;
	SetGamma (gamma);
end

-- Load a default object, for profiling.

--LoadOneObject ("d:\\lancer\\data\\bases\\liberty\\l_bar.cmp");
--LoadOneObject ("d:\\lancer\\data\\bases\\pirate\\p_bar.cmp");
LoadOneObject ("P_fighter.cmp");
--LoadOneObject ("proptest.cmp");

-- We are done

print ("Figure editor started.");
