$debug
-- This is a test of loading a lua file using LuaProfile.dll
TestKey = "This is a test key from Lua.";
TestSection =
{
    audio="filename.wav",
    baseVol=1.0
}

function TestFunc ()
    error ("Executing the test function\n");
end

function TestFunc2 ()
    return "TestFunc2 return.";
end

function TestFunc3 (name, age)
    return name .. " was " .. tostring(age) .. " years old.";
end
