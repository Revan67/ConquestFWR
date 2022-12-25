# Build.pl - A PERL module for building the DA libraries

use Win32 ();
use IO::Handle;

# Ensure that our PERL output is properly interleaved with system() output.
STDOUT->autoflush(1);
STDERR->autoflush(1);

# Print usage if requested
if (grep(/(-h)|(--help)|(\/\?)/i, @ARGV))
{
	print "Usage: build.pl {nowipe|noget|noclean|useenv|out=<logfile>}\n";
	exit;
}

## Build tools configuration
##
$msdevRoot       = "C:\\Progra~1\\Micros~2";
$msdevCommonPath = "$msdevRoot\\Common";
$msdevVC98Path   = "$msdevRoot\\VC98\\Bin";
$msdevBinPath    = "$msdevRoot\\Common\\MSDev98\\Bin";
$msdevToolsPath  = "$msdevRoot\\Common\\Tools";
$msdevOSToolsPath= "$msdevRoot\\Common\\Tools\\WinNT";
$msdevProg       = "$msdevBinPath\\Msdev.exe";

$zipProg         = "\"C:\Program Files\WinZip\WzZIP.EXE\" -Jhrs -o -p -r ";

## SDK configuration
##
$msdevInclude    = "$msdevRoot\\VC98\\Include";
$msdevLib        = "$msdevRoot\\VC98\\Lib";
$msdevMFCInclude = "$msdevRoot\\VC98\\MFC\\Include";
$msdevMFCLib     = "$msdevRoot\\VC98\\MFC\\Lib";
$msdevATLInclude = "$msdevRoot\\VC98\\ATL\\Include";
$msdevATLLib     = "$msdevRoot\\VC98\\ATL\\Lib";

$masmBinPath	 = "c:\\MASM613\\bin;c:\\MASM613\binr";

$dxfRoot         = "c:\\mssdk";
$dxfInclude      = "$dxfRoot\\Include";
$dxfLib          = "$dxfRoot\\Lib";

$dxmRoot         = "c:\\mssdk";
$dxmInclude      = "$dxmRoot\\Include";
$dxmLib          = "$dxmRoot\\Lib";

## Package configuration
##
$packageRoot   = "c:\\dalibs-build";
$packageName   = "dalibs";
$packageOutPath= $packageRoot;

## Build configuration
##
$buildWorkspace= "fullbuild.dsw";
$buildPlatform = "Win32";
$buildConfig   = "Release";
$buildBranch   = "dalibs-1.6.x";
$buildRoot     = "$packageRoot\\libs\\$buildBranch";
$binPath       = "$buildRoot\\Bin";
$libPath       = "$buildRoot\\Lib";
$srcPath       = "$buildRoot\\Src";
$incPath       = "$buildRoot\\Include";
$extLibPath    = "$packageRoot\\External\\Lib";
$extIncludePath= "$packageRoot\\External\\Include";
$buildInclude  = "$buildRoot\\Include;$dxfInclude;$dxmInclude;$msdevInclude;$msdevMFCInclude;$msdevATLInclude;$extIncludePath";
$buildLib      = "$buildRoot\\Lib;$dxfLib;$dxmLib;$msdevLib;$msdevMFCLib;$msdevATLLib;$extLibPath";
$buildToolsPath= "$msdevVC98Path;$msdevCommonPath;$msdevBinPath;$msdevToolsPath;$msdevOSToolsPath;$masmBinPath;";

## Source Control Configuration
##
$vssBinPath    = "$msdevRoot\\Common\\vss\\win32";
$vssProg       = "$vssBinPath\\ss.exe";
$vssUser       = "buildmonkey";
$vssDB         = "\\\\nt\\libs\\tools\\libvss";
$vssRoot       = "\$/libs/$buildBranch";

$LIB_Binaries	= "lib";
$DLL_Binaries	= "lib;dll;pdb";
$Comp_Binaries	= "dll;pdb";
$EXE_Binaries	= "exe;pdb";

## Function to append a project to the list.
##
## 	add_project( dsp_project_filebase, 
##				 dsp_config, 
##				 dsp_config_output_dir, 
##				 dsp_config_output_filebase, 
##				 dsp_config_output_filebase_append, 
##				 dsp_config_output_type )
##
## 	add_project( "COMHeap", "$srcPath/comheap/$buildConfig", "comheap", $LIB_Binaries );
##
sub add_project 
{
	$buildProjList[ $buildProjListCount ] = { 
		name		=> $_[0],
		config		=> $_[1],
		output_dir	=> $_[2],
		filebase	=> $_[3],
		append		=> $_[4],
		type		=> $_[5]
	};

	$buildProjListCount = $buildProjListCount + 1;
}


## Function to distribute the output of the binaries.
##
## dis_project( project_entry_in_project_list )
##
sub dist_project 
{
	$_[0]->{output_dir} =~ s/\//\\\\/g;

	if( $_[0]->{type} eq $LIB_Binaries ) {

		system( "copy $_[0]->{output_dir}\\$_[0]->{filebase}.lib $libPath\\$_[0]->{filebase}$_[0]->{append}.lib");

	}
	elsif( $_[0]->{type} eq $DLL_Binaries ) {

		system( "copy $_[0]->{output_dir}\\$_[0]->{filebase}.lib $libPath\\$_[0]->{filebase}$_[0]->{append}.lib");
		system( "copy $_[0]->{output_dir}\\$_[0]->{filebase}.dll $libPath\\$_[0]->{filebase}$_[0]->{append}.dll");
		system( "copy $_[0]->{output_dir}\\$_[0]->{filebase}.pdb $libPath\\$_[0]->{filebase}$_[0]->{append}.pdb");

	}
	elsif( $_[0]->{type} eq $Comp_Binaries ) {

		system( "copy $_[0]->{output_dir}\\$_[0]->{filebase}.dll $libPath\\$_[0]->{filebase}$_[0]->{append}.dll");
		system( "copy $_[0]->{output_dir}\\$_[0]->{filebase}.pdb $libPath\\$_[0]->{filebase}$_[0]->{append}.pdb");

	}
	elsif( $_[0]->{type} eq $EXE_Binaries ) {

		system( "copy $_[0]->{output_dir}\\$_[0]->{filebase}.exe $binPath\\$_[0]->{filebase}$_[0]->{append}.exe");
		system( "copy $_[0]->{output_dir}\\$_[0]->{filebase}.pdb $binPath\\$_[0]->{filebase}$_[0]->{append}.pdb");

	}
	else {
		die "unknown binarie type $_[0]->{type}";
	}
}


## List of projects to build
##

	$buildProjListCount = 0;
	@buildProjList = ();

	##
	##			 project_name			build_config	output_directory_for_build_config								base_file_name			appended	output_type
	##																																			
																																				
	## LIBs																																		
	##																																			
	add_project( "COMHeap",				$buildConfig,				"$srcPath/comheap/$buildConfig",					"comheap",				"",			$LIB_Binaries );
	add_project( "MathLib",				$buildConfig,				"$srcPath/x86math/$buildConfig",					"mathlib",				"",			$LIB_Binaries );
	add_project( "msgbuffer",			$buildConfig,				"$srcPath/sockets/$buildConfig",					"msgbuffer",			"",			$LIB_Binaries );
	add_project( "RPUL",				$buildConfig,				"$srcPath/RenderPipeline/RPUL/$buildConfig",		"rpul",					"",			$LIB_Binaries );
	add_project( "movielib",			$buildConfig,				"$srcPath/movielib/$buildConfig",					"movielib",				"",			$LIB_Binaries );
	add_project( "wavlib",				$buildConfig,				"$srcPath/wavlib/$buildConfig",						"wavlib",				"",			$LIB_Binaries );
																																							
																																							
	## DLLs	(Implicit)																																		
	##																																						
	add_project( "DACOM",				$buildConfig,				"$srcPath/DACOM/$buildConfig",						"dacom",				"",			$DLL_Binaries );
	add_project( "Sockets",				$buildConfig,				"$srcPath/Sockets/$buildConfig",					"sockets",				"",			$DLL_Binaries );


	## DLLs (Components)
	##				
	add_project( "D3DRenderPipe",		"Release",					"$srcPath/RenderPipeline/DirectDraw/D3D/Release",	"d3drenderpipe",		"",			$Comp_Binaries );
	add_project( "D3DRenderPipe",		"Debug",					"$srcPath/RenderPipeline/DirectDraw/D3D/Debug",		"d3drenderpipe",		"_debug",	$Comp_Binaries );
##	add_project( "GLRenderPipe",		$buildConfig,				"$srcPath/RenderPipeline/GL/Debug",					"glrenderpipe",			"",			$Comp_Binaries );
	add_project( "DOSFile",				$buildConfig,				"$srcPath/DOSFile/$buildConfig",					"dosfile",				"",			$Comp_Binaries );
	add_project( "proto",				$buildConfig,				"$srcPath/x86math/$buildConfig",					"x86math",				"",			$Comp_Binaries );
	add_project( "K62Math",				$buildConfig,				"$srcPath/x86math/$buildConfig",					"k62math",				"",			$Comp_Binaries );
	add_project( "HotKey",				$buildConfig,				"$srcPath/DAHotKey/$buildConfig",					"dahotkey",				"",			$Comp_Binaries );
	add_project( "Anim",				$buildConfig,				"$srcPath/Anim/$buildConfig",						"anim",					"",			$Comp_Binaries );
	add_project( "Channel",				$buildConfig,				"$srcPath/Channel/$buildConfig",					"channel",				"",			$Comp_Binaries );
	add_project( "Materials",			$buildConfig,				"$srcPath/shading/Materials/$buildConfig",			"materials",			"",			$Comp_Binaries );
	add_project( "VertexBufferManager",	$buildConfig,				"$srcPath/VertexBufferManager/$buildConfig",		"vertexbuffermanager",	"",			$Comp_Binaries );
	add_project( "RenderBatcher",		$buildConfig,				"$srcPath/RenderBatcher/$buildConfig",				"renderbatcher",		"",			$Comp_Binaries );
	add_project( "SoundManager",		$buildConfig,				"$srcPath/SoundManager/$buildConfig",				"soundmanager",			"",			$Comp_Binaries );
	add_project( "SoundManager",		"$buildConfig Thread Safe",	"$srcPath/SoundManager/${buildConfig}MT",			"soundmanagermt",		"",			$Comp_Binaries );
	add_project( "Streamer",			$buildConfig,				"$srcPath/Streamer/$buildConfig",					"streamer",				"",			$Comp_Binaries );
	add_project( "DSStreamer",			$buildConfig,				"$srcPath/Streamer/dsstreamer/$buildConfig",		"dsstreamer",			"",			$Comp_Binaries );
	add_project( "System",				$buildConfig,				"$srcPath/System/$buildConfig",						"system",				"",			$Comp_Binaries );
	add_project( "ui",					$buildConfig,				"$srcPath/daui/$buildConfig",						"ui",					"",			$Comp_Binaries );
	add_project( "LuaProfile",			$buildConfig,				"$srcPath/LuaProfile/$buildConfig",					"luaprofile",			"",			$Comp_Binaries );
	add_project( "LightManager",		$buildConfig,				"$srcPath/Shading/LightManager/$buildConfig",		"lightmanager",			"",			$Comp_Binaries );
	add_project( "TextureLibrary",		$buildConfig,				"$srcPath/Shading/TextureLibrary/$buildConfig",		"texturelibrary",		"",			$Comp_Binaries );
	add_project( "TextureLibrary",		"$buildConfig Thread Safe",	"$srcPath/Shading/TextureLibrary/${buildConfig}MT",	"texturelibrarymt",		"",			$Comp_Binaries );
	add_project( "defcomp",				$buildConfig,				"$srcPath/defcomp/$buildConfig",					"deformable",			"",			$Comp_Binaries );

	add_project( "DataViewer",			$buildConfig,				"$srcPath/viewer/docuview/$buildConfig",			"docuview",				"",			$Comp_Binaries );
	add_project( "HexView",				$buildConfig,				"$srcPath/viewer/HexView/$buildConfig",				"hexview",				"",			$Comp_Binaries );
	add_project( "VfxView",				$buildConfig,				"$srcPath/viewer/VfxView/$buildConfig",				"vfxview",				"",			$Comp_Binaries );

	add_project( "Engine",				$buildConfig,				"$srcPath/Engine/$buildConfig",						"engine",				"",			$Comp_Binaries );
	add_project( "EngineLights",		$buildConfig,				"$srcPath/EngComps/EngineLights/$buildConfig",		"enginelights",			"",			$Comp_Binaries );
	add_project( "EngineCameras",		$buildConfig,				"$srcPath/EngComps/EngineCameras/$buildConfig",		"enginecameras",		"",			$Comp_Binaries );
	add_project( "HardPoint",			$buildConfig,				"$srcPath/EngComps/HardPoint/$buildConfig",			"hardpoint",			"",			$Comp_Binaries );
	add_project( "Trap",				$buildConfig,				"$srcPath/EngComps/Physics/ODE/Trap/$buildConfig",	"trap",					"",			$Comp_Binaries );
	add_project( "Property",			$buildConfig,				"$srcPath/EngComps/Property/$buildConfig",			"property",				"",			$Comp_Binaries );
	add_project( "Physics",				$buildConfig,				"$srcPath/EngComps/Physics/$buildConfig",			"physics",				"",			$Comp_Binaries );
	add_project( "Collision",			$buildConfig,				"$srcPath/EngComps/Collision/$buildConfig",			"collision",			"",			$Comp_Binaries );
	add_project( "RenderMgr",			$buildConfig,				"$srcPath/EngComps/RenderMgr/$buildConfig",			"rendermgr",			"",			$Comp_Binaries );
	add_project( "PolyMesh",			$buildConfig,				"$srcPath/RendComp/PolyMesh/$buildConfig",			"polymesh",				"",			$Comp_Binaries );
	add_project( "TriMesh",				$buildConfig,				"$srcPath/RendComp/TriMesh/$buildConfig",			"trimesh",				"",			$Comp_Binaries );
##	add_project( "NURBMesh",			$buildConfig,				"$srcPath/RendComp/NURBMesh/$buildConfig",			"nurbmesh",				"",			$Comp_Binaries );
	add_project( "Optics",				$buildConfig,				"$srcPath/RendComp/Optics/$buildConfig",			"optics",				"",			$Comp_Binaries );

	
	## EXEs
	##
	add_project( "ResTool",				$buildConfig,				"$srcPath/Tools/ResTool/$buildConfig",				"restool",				"",			$EXE_Binaries );
	add_project( "char",				$buildConfig,				"$srcPath/Tools/DeformView/$buildConfig",			"char",					"",			$EXE_Binaries );
	add_project( "CmpndView",			$buildConfig,				"$srcPath/Tools/ObjView/$buildConfig",				"objview",				"",			$EXE_Binaries );
	add_project( "dbgtee",				$buildConfig,				"$srcPath/Tools/dbgtee/$buildConfig",				"dbgtee",				"",			$EXE_Binaries );
	add_project( "dainfo",				$buildConfig,				"$srcPath/Tools/dainfo/$buildConfig",				"dainfo",				"",			$EXE_Binaries );
	add_project( "HotkeyEditor",		$buildConfig,				"$srcPath/Tools/HotkeyEditor/$buildConfig",			"hotkeyeditor",			"",			$EXE_Binaries );
	add_project( "mergeanm",			$buildConfig,				"$srcPath/Tools/mergeanm/$buildConfig",				"mergeanm",				"",			$EXE_Binaries );
	add_project( "pack",				$buildConfig,				"$srcPath/Tools/pack/$buildConfig",					"pack",					"",			$EXE_Binaries );
	add_project( "ParticleEditor",		$buildConfig,				"$srcPath/Tools/ParticleEditor/$buildConfig",		"particleeditor",		"",			$EXE_Binaries );
	add_project( "phyedit",				$buildConfig,				"$srcPath/Tools/PhysicsEditor/$buildConfig",		"phyedit",				"",			$EXE_Binaries );
	add_project( "PropEdit",			$buildConfig,				"$srcPath/Tools/PropEdit/$buildConfig",				"propedit",				"",			$EXE_Binaries );
	add_project( "traceout",			$buildConfig,				"$srcPath/Tools/traceout/$buildConfig",				"traceout",				"",			$EXE_Binaries );
	add_project( "TreeView",			$buildConfig,				"$srcPath/Tools/TreeView/$buildConfig",				"treeview",				"",			$EXE_Binaries );
	add_project( "txmlib",				$buildConfig,				"$srcPath/Tools/Exporters/Common/$buildConfig",		"txmlib",				"",			$EXE_Binaries );
	add_project( "UTFApp",				$buildConfig,				"$srcPath/Tools/ViewUTF/$buildConfig",				"vu",					"",			$EXE_Binaries );
	add_project( "UTFApp",				"Edit$buildConfig",			"$srcPath/Tools/ViewUTF/Edit$buildConfig",			"vue",					"",			$EXE_Binaries );
	add_project( "utftool",				$buildConfig,				"$srcPath/Tools/utftool/$buildConfig",				"utftool",				"",			$EXE_Binaries );


###
### Should not have to edit anything below here
###

	$rmDirCmd = "deltree /y";
	if (Win32::IsWinNT())
	{
		$rmDirCmd = "rd /s /q";
	}

	$buildToolsPath .= $ENV{"PATH"};
	$ENV{"PATH"}     = $buildToolsPath;
	$ENV{"INCLUDE"}  = $buildInclude;
	$ENV{"LIB"}      = $buildLib;
	$ENV{"SSUSER"}   = $vssUser;
	$ENV{"SSDIR"}    = $vssDB;

	print "============ BEGIN ENV DUMP ============\n";
	print "PATH = " ;
	print $ENV{"PATH"};
	print "\n";
	print "INCLUDE = " ;
	print $ENV{"INCLUDE"};
	print "\n";
	print "LIB = " ;
	print $ENV{"LIB"};
	print "\n";
	print "============   END ENV DUMP ============\n";

	chdir ($buildRoot);

	# Clear out the include, implicit, explicit, static, and src directories, to ensure that no old
	# libraries, headers, or source are used.
	#

	if (!grep(/nowipe/i, @ARGV))
	{
		print "============ BEGIN DIRECTORY CLEANSING ============\n";
		system ("$rmDirCmd $binPath");
		system ("$rmDirCmd $libPath");
		system ("$rmDirCmd $srcPath");
		system ("$rmDirCmd $incPath");
		system ("$rmDirCmd $extLibPath");
		system ("$rmDirCmd $extIncludePath");
		print "============  END DIRECTORY CLEANSING  ============\n";
	}

	print "============ BEGIN DIRECTORY CREATION ============\n";
	mkdir ($binPath,		 o777);
	mkdir ($libPath,		 o777);
	mkdir ($incPath,		 o777);
	mkdir ($srcPath,		 o777);
	mkdir ($extLibPath,		 o777);
	mkdir ($extIncludePath,  o777);
	print "============   END DIRECTORY CREATION ============\n";


	# Retrieve the entire development tree from source safe
	#
	if (!grep(/noget/i, @ARGV))
	{
		print "============ BEGIN SOURCE CODE RETRIEVAL ============\n";
		system ($vssProg, "Get $vssRoot -GF -I-Y");
		system ($vssProg, "Get $vssRoot/$buildWorkspace -GF -I-Y");
		system ($vssProg, "Get $vssRoot/include -GF -R -I-Y");
		system ($vssProg, "Get $vssRoot/src -GF -R -I-Y");
		system ($vssProg, "Get \$/external/include -GF -R -I-Y");
		system ($vssProg, "Get \$/external/lib -GF -R -I-Y");
		print "============  END SOURCE CODE RETRIEVAL  ============\n";
	}

	if( !grep(/noversion/i, @ARGV) ) 
	{
	# Lock the library version file and increment the build number within that file.
	# Only if building the latest version
	#

	$oldLibVerFile = "$buildRoot\\include\\libver.h.old";
	$libVerFile = "$buildRoot\\include\\libver.h";

	system ("copy $libVerFile $oldLibVerFile");
	system ($vssProg, "Checkout $vssRoot/include/libver.h -GF -I-N");

	open (OLDLIBVER, "<$oldLibVerFile") or die "Failed to open $oldLibVerFile for reading.\n";
	open (LIBVER, ">$libVerFile") or die "Failed to open $libVerFile for writing.\n";

	while (<OLDLIBVER>)
	{
		if (/#define\s+LIB_BUILD\s+(\d+)\s*$/)
		{
			# Re-write this line with the incremented version number.		
			$buildNum = $1;
			$buildNum += 1;
			print LIBVER "#define LIB_BUILD $buildNum\n";
		}
		elsif (/#define\s+LIB_VER_STRING\s+"(\d+\.\d+)\.(\d+)\\0"/)
		{
			# Re-write this line with the incremented version number string, preserving the major and minor
			$libVersion = "$1.$buildNum";
			print LIBVER "#define LIB_VER_STRING \"$libVersion\\0\"\n";
		}
		else
		{
			# Simply echo this line back out.
			print LIBVER $_;
		}
	}

	close OLDLIBVER;
	close LIBVER;
	}

	#
	# Perform the building process, optionally cleaning them all before proceeding.
	# 

	print "============ BEGIN BUILD ============\n";
	print "Configuration:   \"$buildConfig\"\n";
	print "Library version: $libVersion\n";

	# Perform the make process. This is complicated by the fact that there are internal dependencies between the
	# projects in the workspace that build static or import libraries, and those that just build component DLLs.
	# Instead of trying to figure out the dependencies, you are required to list them in the order they should be
	# built.

	print "*** Building...\n";

	foreach my $proj (@buildProjList) 
	{
		$cfg = $proj->{config};
		if (!$cfg)
		{
			$cfg = $buildConfig;
		}
		
		$cmd = "$msdevProg $buildWorkspace /MAKE \"$proj->{name} - $buildPlatform $cfg\" /USEENV";
		
		print "+++ Building $proj->{name} - $buildPlatform $cfg\n";
		## print "+++ cmd: $cmd\n";

		$response = `$cmd`;
		$output .= $response;
		print $response ;
		print "\n";

		dist_project( $proj );

#		# Execute the post build step for this project before proceeding.
#		# Convert sane slashes to insane slashes.
#		$proj->{postbuild} =~ s/\//\\\\/g;
#		eval ($proj->{postbuild});
	}

	print "*** Build Done.\n";


	print "*** Build output ***\n";
	if (grep (/out=\s*([^\s]+)\s*/i, @ARGV))
	{
		print "Sending output to \"$1\"\n";
		if (open (OUTFILE, ">$1"))
		{
			print OUTFILE $output;
			close (OUTFILE);
		}
	}
	else
	{
		print "Failed to find an output file.\n";
	}

	print "*** Stamping Versions (stamp, stamp, stamp) ***\n";

	$resSrc = "$libPath\\DACOM.DLL/RT_VERSION:#1";  
	system( "$binPath\\restool.exe $libPath\\*.dll -a $resSrc" );
	system( "$binPath\\restool.exe $binPath\\*.exe -a $resSrc" );

	print "============ END BUILD ============\n";


	# Determine if the build succeeded or not.
	# This is done by looking for the error count for the
	#
	$errorCount = 0;
	while ($output =~ /^.*(\d+)\s+error\(s\).*$/igm)
	{
		if ($1 != 0)
		{
			$errorCount += $1;
			print "$&\n";
		}
	}

	$buildResult = " - GOOD";
	if ($errorCount > 0)
	{
		print "There were $errorCount total errors.\n";
		$buildResult = " - BAD";
	}

	if( !grep(/noversion/i, @ARGV) ) 
	{
		system ($vssProg, "Checkin $vssRoot/include/libver.h -GF	-C-");
	}

	# Label the build
	system ($vssProg, "Label $vssRoot/ -C -I- -L\"LibVer$libVersion$buildResult\"");
	system ($vssProg, "Label \$/external/ -C -I- -L\"LibVer$libVersion$buildResult\"");

	# If there are no errors, and there is a valid publish directory, start the publishing process
	if ($packageOutPath && $errorCount == 0)
	{
		## Publish the build.
		##

		$publishPath = "$packageOutPath\\$packageName";

		print "*** Publishing to $publishPath\n";
		
		print "+++ Removing $publishPath recursively\n";
		$output = `$rmDirCmd $publishPath`;
		print $output;
		
		print "+++ Creating directories\n";
		$output = `md $publishPath\\Bin`;
		$output = `md $publishPath\\Lib`;
		$output .= `md $publishPath\\Include`;
		print $output;

		print "+++ Copying files\n";
		system ("xcopy /s $binPath\\*.* $publishPath\\Bin");
		system ("xcopy /s $libPath\\*.* $publishPath\\Lib");
		system ("xcopy /s $extLibPath\\*.* $publishPath\\Lib");
		system ("xcopy /s $incPath\\*.* $publishPath\\Include");
		
		print "*** Publishing completed.\n";

##	This does not work
##
##		## Archive the build.
##		##
##
##		$packageArchiveFile = "$packageOutPath\\$packageName-$libVersion.zip";
##
##		print "*** Archiving $publishPath in $packageArchiveFile\n";
##
##		print "+++ Creating archive\n";
##		chdir( $publishPath );
##		system( $vssProg, "Get \$/Distributions/dalibs -GF -R -I-N" );
##		system( "del /Q $packageArchiveFile" );
##		system( "del /Q /S $publishPath\\*.scc" );
##		system( "del /Q /S $publishPath\\Include\\libver.h.old" );
##		system( "attrib +R /S $publishPath\\*.*" );
##		$output = "$zipProg $packageArchiveFile $publishPath\\*.*";
##		print $output;
##
##		print "\n*** Archiving completed.\n";
	}

exit;
