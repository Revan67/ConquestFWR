# Distrib.pl - A PERL program for distributing a library build based on a crontab-like distribution list

# The syntax of the distribution file is:
#    One entry per line.
#    Empty lines and lines starting with '#' are ignored.
#    Each line has the syntax: <day> <destDir> <contact>
#      where <day> is the weekday to distribute, 0-6, 0==Sunday, 6==Saturday
#            <destDir> is the directory in which to put the distribution
#            <contact> is the email address to send the results to.
#      The <day> can be a * for every day, a single number, a series of numbers seperated by commas, or a range.
#      E.g. * 0 1,2,4 1-5

use Win32 ();
use IO::Handle;

#
# Subroutines
#

###############################################################################
# in_range checks for a match within a range spec
###############################################################################

sub in_range
{
    local ($what, $rangeSpec) = @_;
    local ($minVal, $maxVal) = split(/-/, $rangeSpec, 2);

	# Only check valid ranges
	if ($minVal < $maxVal)
	{
		if ( ($what ge $minVal) && ($what le $maxVal) )
		{
			return 1;
		}
	}
    return 0;
}

###############################################################################
# in_csl searches for an element in a comma separated list
###############################################################################

sub in_csl
{
    local ($what, $csl) = @_;
    @a = split(/,/, $csl);
    for $x (@a)
	{
		if (($what eq $x) || &in_range($what, $x))
		{
			return 1;
		}
    }
    return 0;
}

sub MSG
{
	print @_;
}

#
# Main program start
#

# Ensure that our PERL output is properly interleaved with system() output.
STDOUT->autoflush(1);
STDERR->autoflush(1);

# Set the directory command, which is different for NT and Win9x
$rmDirCmd = "deltree /y";
if (Win32::IsWinNT())
{
	$rmDirCmd = "rd /s /q";
}

# Print usage if requested
if (grep(/(-h)|(--help)|(\/\?)/i, @ARGV) || @ARGV < 1)
{
	print "Usage: distrib.pl <distribfile>\n";
	print "Distributes a library build according to the information in <distribfile>.\n";
	print "The \"BUILD_PUBLISH_PATH\" environment variable is used as the source for the distibution.\n";
	exit;
}

# Retrieve the environmental configuration values. Set those that are empty to default values

$publishPath = $ENV{"BUILD_PUBLISH_PATH"};
if (!$publishPath)
{
	print "BUILD_PUBLISH_PATH undefined. Libraries will not be distributed.\n";
	exit;
}

# Get the current time in a format we can use.
# mon = 0..11 and wday = 0..6
($sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst) = 
	localtime(time);
$mon++; # to get it to agree with the cron syntax

# Attempt to open the distribution list, exiting on failure.
$distList = $ARGV[0];

open (DISTLIST, "<$distList") or die "Failed to open $distList for reading.\n";
while (<DISTLIST>)
{
	MSG "Line: $_";

	# remove the trailing newline before processing. We don't need it.
	chomp;

	# Skip blank and comment lines
   	if (/^$/) {
		MSG("Skipping blank line\n");
	    next;
   	}

   	if (/^#/) {
		MSG("Skipping comment line\n");
	    next;
   	}

	# Split out the weekday and destDir fields
   	($twday, $tdestDir, $contact) = split(/ +/, $_, 3);
	MSG("Spec Line: $twday, $tdestDir, $contact\n");

	# Check the weekday spec against the current weekday. If there is a match, distribute the library.
   	if ( ($twday eq "*") || ($wday == $twday) || &in_csl($wday, $twday) || &in_range($wday, $twday) )
	{
		MSG("Found a match\n");
		# Do it
		$output = "*** Distributing to $tdestDir starting...\n";
		
		# Delete any backup copy
		$output .= "+++ Removing old copy of $tdestDir ($tdestDir.1) recursively\n";
		$output .= `$rmDirCmd $tdestDir.1`;

		# Rename the current copy
		$output .= "+++ Renaming $tdestDir to $tdestDir.1\n";
		$output .= `move $tdestDir $tdestDir.1`;

		# Create the new directory and copy the distribution
		$output .= "+++ Creating directory $tdestDir\n";
		$output .= `md $tdestDir`;

		# Finally, copy the files
		$output .= "+++ Copying files\n";
		$output .= `xcopy /s $publishPath\\*.* $tdestDir`;
		$output .= "*** Distribution to $tdestDir complete.\n";
		MSG($output);

		# Notify the contact about the distribution.
		$mailCmd = "perl sendmail.pl $contact\@digitalanvil.com tbratton\@digitalanvil.com \"Library Distribution Results...\"";
		if (open (RESULT, "| $mailCmd"))
		{
			MSG("Sending mail...");
			print RESULT $output;
			close (RESULT);
			MSG("Done.\n");
		}
		else
		{
			MSG("Failed to open mail command.\n");
		}
	}
}

close (DISTLIST);
			