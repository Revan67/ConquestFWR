# Perl script to send mail from STDIN
# Usage: perl sendmail.pl <destaddr> <srcaddr> {<subject>}
# Sends from STDIN.

#------------------------------------------------------------
# sub sendmail()
#
# send/fake email around the world ...
#
# Version : 1.21
# Environment: Hip Perl Build 105 NT 3.51 Server SP4
# Environment: Hip Perl Build 110 NT 4.00
#
# arguments:
#
# $from email address of sender
# $reply email address for replying mails
# $to email address of reciever
# (multiple recievers can be given separated with space)
# $smtp name of smtp server (name or IP)
# $subject subject line
# $message (multiline) message
#
# return codes:
#
# 1 success
# -1 $smtphost unknown
# -2 socket() failed
# -3 connect() failed
# -4 service not available
# -5 unspecified communication error
# -6 local user $to unknown on host $smtp
# -7 transmission of message failed
# -8 argument $to empty
#
# usage examples:
#
# print
# sendmail("Alice <alice\@company.com>",
# "alice\@company.com",
# "joe\@agency.com charlie\@agency.com",
# $smtp, $subject, $message );
#
# or
#
# print
# sendmail($from, $reply, $to, $smtp, $subject, $message );
#
# (sub changes $_)
#
#------------------------------------------------------------

use Socket;

sub sendmail {

my ($from, $reply, $to, $smtp, $subject, $message) = @_;

my ($fromaddr) = $from;
my ($replyaddr) = $reply;

$to =~ s/[ \t]+/, /g; # pack spaces and add comma
$fromaddr =~ s/.*<([^\s]*?)>/$1/; # get from email address
$replyaddr =~ s/.*<([^\s]*?)>/$1/; # get reply email address
$replyaddr =~ s/^([^\s]+).*/$1/; # use first address
$message =~ s/^\./\.\./gm; # handle . as first character
$message =~ s/\r\n/\n/g; # handle line ending
$message =~ s/\n/\r\n/g;
$smtp =~ s/^\s+//g; # remove spaces around $smtp
$smtp =~ s/\s+$//g;

if (!$to) { return -8; }

my($proto) = (getprotobyname('tcp'))[2];
my($port) = (getservbyname('smtp', 'tcp'))[2];

my($smtpaddr) = ($smtp =~
/^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/)
? pack('C4',$1,$2,$3,$4)
: (gethostbyname($smtp))[4];

if (!defined($smtpaddr)) { return -1; }

if (!socket(S, AF_INET, SOCK_STREAM, $proto)) { return
-2; }
if (!connect(S, pack('Sna4x8', AF_INET, $port, $smtpaddr))) { return			  +
-3; }

my($oldfh) = select(S); $| = 1; select($oldfh);

$_ = <S>; if (/^[45]/) { close S; return -4; }

print S "helo localhost\r\n";
$_ = <S>; if (/^[45]/) { close S; return -5; }

print S "mail from: <$fromaddr>\r\n";
$_ = <S>; if (/^[45]/) { close S; return -5; }

foreach (split(/, /, $to)) {
print S "rcpt to: <$_>\r\n";
$_ = <S>; if (/^[45]/) { close S; return -6; }
}

print S "data\r\n";
$_ = <S>; if (/^[45]/) { close S; return -5; }

print S "To: $to\r\n";
print S "From: $from\r\n";
print S "Reply-to: $replyaddr\r\n" if $replyaddr;
print S "X-Mailer: Perl Sendmail Version 1.21 Christian Mallwitz
Germany\r\n";
print S "Subject: $subject\r\n\r\n";
print S "$message";
print S "\r\n.\r\n";

$_ = <S>; if (/^[45]/) { close S; return -7; }

print S "quit\r\n";
$_ = <S>;

close S;
return 1;
}

# Main program
use Mail::Send;

$destaddr = $ARGV[0];
$srcaddr = $ARGV[1];
$subject = $ARGV[2];

$msg = new Mail::Send;

if (!$destaddr || !$srcaddr)
{
	die "Usage: perl sendmail.pl <to> <from> {<subject>}\n";
}

if (!$subject)
{
	$subject = "";
}

$message = "";
while (<STDIN>)
{
	$message .= $_;
}

print sendmail ($srcaddr, $srcaddr, $destaddr, "iron.digitalanvil.com", $subject, $message);
