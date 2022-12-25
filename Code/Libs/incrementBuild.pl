#
# IncrementBuild.pl - Filters the input stream to the output, incrementing the build number and adjusting the build
# version string.
#

$batchName = $ARGV[0];
($batchName ne "") or die "Usage: perl incrementBuild.pl <batch file name>\n";

while (<STDIN>)
{
	if (/#define LIB_BUILD.+(\d+)$/)
	{
		$buildNum = $1;
		$buildNum += 1;
		print "#define LIB_BUILD $buildNum\n";
	}
	elsif (/#define LIB_VER_STRING "(\d+\.\d+)\.(\d+)\\0"/)
	{
		$libVersion = "$1.$buildNum";
		print "#define LIB_VER_STRING \"$libVersion\\0\"\n";
	}
	else
	{
		print $_;
	}
}

# Write out the turd, uh, the batch file.
open (BATCHFILE, ">$batchName") or die "Unable to open output batch file \"$batchName\"\n";
print BATCHFILE "set BUILD_VERSION=$libVersion\n";
close(BATCHFILE);
