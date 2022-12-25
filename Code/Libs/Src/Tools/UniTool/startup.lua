-- Menus can be added the application at will here.
-- The syntax for the AddMenu command is:
-- AddMenu (MenuItem <parent>)
-- AddMenuItem (string <menu name>, Menu <owner>, number <position>, function <action>)
-- The action function will be called when the menu is selected.
$debug

function TestMenu1 ( )
	execute ("telnet.exe");
end

function TestMenu2 ( )
	execute ("edit.com");
end

function ShowFile (filename)
	execute ("start "..filename);
end

function ShowHelp ( )
	execute ("Winhelp");
end

function DumpString (text)
	dumpwin.text_widget:set_text(dumpwin.text_widget:get_text()..text);
end

function DumpTable (table, tableName)
	-- Print out the table elements, recursing into sub-tables
	local i, value;
	i, value = next(table, nil);
	while i ~= nil do
		DumpString(tableName.."."..tostring(i).." == ");
		if type(value) == "table" then
			DumpString("[TABLE]\r\n");
			DumpTable (value, tableName.."."..tostring(i));
		else
			DumpString(tostring(value).."\r\n");
		end
		i, value = next (table, i);
	end
end

function DumpVars ()
	-- Create a new window to display the output, if not already there
	if dumpwin == nil then
		dumpwin = NewTopLevel ("Variable Dump", 0, 0, 600, 200);
		dumpwin.text_widget = NewText (dumpwin, "", 0, 0, dumpwin:get_width(), dumpwin:get_height());
		dumpwin.OnSize = function (widget, eventName)
			widget.text_widget:set_size (widget:get_width(), widget:get_height());
		end
	end

	dumpwin.text_widget:set_text("All LUA Variables:\r\n");

	local varName, varValue;
	varName , varValue = nextvar (nil);
	while varName ~= nil do
		DumpString(varName.." == ");
		if type(varValue) == "table" then
			DumpString("[TABLE]\r\n");
			DumpTable (varValue, varName);
		else
			DumpString(tostring(varValue).."\r\n");
		end
		varName, varValue = nextvar (varName);
	end
end

-- LUA Code Execution Window.
CodeWin = NewTopLevel("LUA Code Executor", 0, 0, 600, 200);
CodeWin:set_menu(NewMenu());
CodeWin:set_visible(nil);
BogusMenu = AddMenu(CodeWin, AddMenuItem(CodeWin, "&Bogus", CodeWin:get_menu(), -1, nil));
AddMenuItem
(
	CodeWin,
	"&Telnet",
	BogusMenu,
	1,
	function ()
		local title = CodeWin:get_text();
		CodeWin:set_text ("Running telnet...");
		execute ("telnet.exe arrakis");
		CodeWin:set_text (title);
	end
);

AddMenuItem
(
	CodeWin,
	"&New Button",
	BogusMenu,
	2,
	function ()
		local b = NewButton
		(
			CodeWin,
			"Hide This Window",
			0,120
		);
		b.OnPress = function (widget, eventName)
			CodeWin:set_visible (nil);
		end
	end
);

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

function MakeWindowToggleFunc (window)
    return function ()
		if %window:get_visible() then
			%window:set_visible(nil);
		else
			%window:set_visible(1);
		end
	end
end

function ToggleCodeWindow()
	if CodeWin:get_visible() then
		CodeWin:set_visible (nil);
	else
		CodeWin:set_visible (1);
	end
end

-- Render window tests

function SetupRenderer (width, height, multiple)
	CreateRender ("Test Render Window", 0, 0, width, height);
	if (multiple) then
		cameras = 
		{
			NewCamera (0,       0,        width/2, height/2),
			NewCamera (width/2, 0,        width/2, height/2),
			NewCamera (width/2, height/2, width/2, height/2),
			NewCamera (0,       height/2, width/2, height/2),
		};
	else
		cameras = { NewCamera (0,       0,        width,   height) };
	end

	camera = cameras[1];

--	object = NewObject ("sphere.3db");
	object = NewObject ("l_elite.cmp");

	RenderWindow.OnSize = function (widget, eventName)
		local w, h;
		w = RenderWindow:get_width();
		h = RenderWindow:get_height();

		if (%multiple) then
			cameras[1]:set_viewport (0,     0,     w/2, h/2);
			cameras[2]:set_viewport (w/2,   0,     w/2, h/2);
			cameras[3]:set_viewport (w/2,   h/2,   w/2, h/2);
			cameras[4]:set_viewport (0,     h/2,   w/2, h/2);
		else
			camera:set_viewport (0, 0, w, h);
		end
		RenderWindow.zoomscroll:set_size
		(
			RenderWindow:get_width() - RenderWindow.zoomscroll:get_x(),
			RenderWindow.zoomscroll:get_height()
		);
	end

	local initPos = Vector(0,0,20);

	if (multiple) then
		cameras[1]:set_pos (initPos);
		cameras[2]:set_pos (initPos);
		cameras[3]:set_pos (initPos);
		cameras[4]:set_pos (initPos);
	else
		camera:set_pos (initPos);
	end
--	RenderWindow:set_visible(nil);
end

SetupRenderer(320, 240, nil);

RenderWindow.animate = NewButton
(
	RenderWindow,
	"Animate Camera",
	10, 0
);

RenderWindow.animate.OnPress = function (widget, eventName)
	zpos = 0.5;
	pos = Vector(0,0,zpos);
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
RenderWindow.quit.OnPress = function (widget, eventName)
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

-- Main window menus

EditMenu = AddMenu(MainWindow, AddMenuItem(MainWindow, "&Edit", MainMenu, -1, nil));
ViewMenu = AddMenu(MainWindow, AddMenuItem(MainWindow, "&View", MainMenu, -1, nil));
EventMenu = AddMenu(MainWindow, AddMenuItem(MainWindow, "E&vents", MainMenu, -1, nil));
DebugMenu = AddMenu(MainWindow, AddMenuItem(MainWindow, "&Debug", MainMenu, -1, nil));
-- HelpMenu = AddMenu(MainWindow, AddMenuItem(MainWindow, "Help", MainMenu, -1, TestMenu3));
AddMenuItem(MainWindow, "&Help", MainMenu, -1, ShowHelp);
AddMenuItem(MainWindow, "Cu&t", EditMenu, 1, TestMenu1);
AddMenuItem(MainWindow, "&Copy", EditMenu, 2, TestMenu3);
AddMenuItem (MainWindow, "&Dump", DebugMenu, 1, DumpVars);
AddMenuItem (MainWindow, "&Toggle Code Window", DebugMenu, 2, MakeWindowToggleFunc(CodeWin));
AddMenuItem (MainWindow, "&Toggle Render Window", DebugMenu, 2, MakeWindowToggleFunc(RenderWindow));

TestButton = NewButton(MainWindow, "Test Button", 10, 10);
TestLabel = NewLabel(MainWindow, "This is a test of the label window", 10, 200, 400, 50);
TestScroll = NewScroll(MainWindow, 0, 400, 400, 20, "h");
TestScroll.OnChange = function (widget, eventName)
	local min, max, pos;
	min = widget:get_scroll_min();
	max = widget:get_scroll_max();
	pos = widget:get_scroll_pos();
	print ("Scroll pos:", pos, "Scroll Range", min, max);
end

TestList = NewListBox (MainWindow, 400, 0, 200, 100, "s");
i = 0;
while i < 15 do
	TestList:add_string("Test"..tostring(i));
	i = i + 1;
end

TestList.OnChange = function (w, ev)
	local count = w:get_select_count();
	local i = 0;
	print ("Listbox:onChange");
	while i < count do
		print("Sel#"..tostring(i), w:get_item_string(w:get_select(i)));
		i = i+1;
	end
end

TestList.OnDoubleClick = function (w, ev)
	local count = w:get_select_count();
	local i = 0;
	print ("Listbox:onDoubleClick");
	while i < count do
		print("DoubleClick#"..tostring(i), w:get_item_string(w:get_select(i)));
		i = i+1;
	end
end

TestCombo = NewComboBox (MainWindow, 200, 300, 200, 100, "l");
i = 0;
while i < 15 do
	TestCombo:add_string("ComboTest"..tostring(i));
	i = i + 1;
end

TestCombo.OnChange = function (w, ev)
	print ("Combobox:onChange");
	print("Sel#"..tostring(i), w:get_text());
end

TestCombo.OnDoubleClick = function (w, ev)
	print ("Combobox:onDoubleClick");
	print("DoubleClick#"..tostring(i), w:get_text());
end

TestCombo:set_select(0);
TestCombo:set_enabled(nil);

TestButton.OnPress = function (widget, eventName)
	-- AddMenuItem(eventName, EventMenu, -1, nil);
	-- Deactivate this button from further presses.
	-- widget.OnPress = nil;
	local w = Shorts:get_width();
	local h = Shorts:get_height();
--	Shorts:set_size (w - 10, h - 10);
	local x = Shorts:get_x();
	local y = Shorts:get_y();
--	print ("X=="..tostring(x));
--	print ("Y=="..tostring(y));
	Shorts:set_position (x + 5, y + 5);
	if Shorts:get_enabled() then
		Shorts:set_enabled(nil);
	elseif Shorts:get_visible() then
		Shorts:set_visible(nil);
	else
		Shorts:set_visible(1);
		Shorts:set_enabled(1);
	end
end

Shorts = NewButton(MainWindow, "Eat My Shorts", 10, 100);
Shorts.OnPress = function (widget, eventName)
--	print ("This is a test."..tostring(widget));
	local w = widget:get_width();
--	print ("get_width() returned:"..tostring(w));
	local h = widget:get_height();
--	print ("get_height() returned:"..tostring(h));
	widget:set_size (w + 10, h + 10);
	local text = widget:get_text();
	print ("Text is"..tostring(text));
	widget:set_text (text..", with mustard!");
	widget:set_size (widget:get_prefwidth(), widget:get_prefheight());
	TestLabel:set_text(text..", with mustard!");
end

TestImage = NewImage (MainWindow, ".\\pastoral.bmp", 50, 50);


-- Frame widget tests
$if nil
tf = NewFrame
(
	NewTopLevel
	(
		"Frame Test Window",
		320,200,
		640,480
	),
	320,200,
	320,240
);

NewButton
(
	tf,
	"Test Button",
	0,0
);
$end

