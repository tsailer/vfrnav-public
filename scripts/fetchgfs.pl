#!/usr/bin/perl

require HTTP::Request;
require HTTP::Response;
require LWP::UserAgent;
use File::Path;

my $gfsdir = $ENV{"HOME"} . "/.vfrnav/gfs";
my $baseurl = 'http://www.ftp.ncep.noaa.gov/data/nccf/com/gfs/prod/';

mkpath($gfsdir);

my $ua = LWP::UserAgent->new;
my $response;
for (my $retry = 0; ; ++$retry) {
    my $request = HTTP::Request->new(GET => $baseurl);
    $response = $ua->request($request);
    if ($response->is_success) {
	last;
    }
    my $ln = $response->status_line;
    if ($retry >= 10) {
	print STDERR "Cannot get GFS directory $baseurl: $ln\n";
	exit 1;
    }
    print STDERR "Retry fetching GFS directory $baseurl: $ln\n";
    sleep(10);
}
#print $response->decoded_content;
my @subpaths;
my @gfsfiles;

my $htmlp = HTML::Parser->new(api_version => 3);
$htmlp->handler( start => \&start_handler_1, "tagname,attr");
$htmlp->parse($response->decoded_content);
$htmlp->eof;

foreach $url (@subpaths) {
    for (my $retry = 0; ; ++$retry) {
	my $request = HTTP::Request->new(GET => $url);
	$response = $ua->request($request);
	if ($response->is_success) {
	    last;
	}
	my $ln = $response->status_line;
	if ($retry >= 10) {
	    print STDERR "Cannot get GFS subdirectory $url: $ln\n";
	    exit 1;
	}
	print STDERR "Retry fetching GFS subdirectory $url: $ln\n";
	sleep(10);
    }
    $htmlp = HTML::Parser->new(api_version => 3);
    $htmlp->handler( start => \&start_handler_2, "tagname,attr");
    $htmlp->parse($response->decoded_content);
    $htmlp->eof;
}

@gfsfiles = sort { $b cmp $a } @gfsfiles;

foreach $url (@gfsfiles) {
    $url =~ m,/+([^/]+)/+([^/]+)$,;
    my $dir = $1;
    my $file = $2;
    print "Result: $url $dir $file\n";
}

if (scalar @gfsfiles < 1) {
    print STDERR "No GFS files found\n";
    exit 1;
}

$url = $gfsfiles[0];
$url =~ s/00$//;
$url =~ m,/+([^/]+)/+([^/]+)$,;
my $dir = $1;
my $filebase = $2;
mkpath($gfsdir . "/" . $dir);

$ua->show_progress(TRUE);
for (my $hr = 0; $hr <= 192; $hr += 3) {
    my $hrstr = sprintf("%02u", $hr);
    my $tmpfile = $gfsdir . "/" . $dir . "/." . $filebase . $hrstr;
    my $endfile = $gfsdir . "/" . $dir . "/" . $filebase . $hrstr;
    next if -r $endfile;
    print STDOUT "Downloading ${dir}/${filebase}${hrstr}\n";
    for (my $retry = 0; ; ++$retry) {
	my $request = HTTP::Request->new(GET => $url . $hrstr);
	$response = $ua->request($request, $tmpfile);
	if ($response->is_success) {
	    link($tmpfile,$endfile);
	    last;
	}
	my $ln = $response->status_line;
	if ($retry >= 10) {
	    print STDERR "Cannot get GFS file ${filebase}${hrstr}: $ln\n";
	    last;
	}
	print STDERR "Retry fetching GFS file ${filebase}${hrstr}: $ln\n";
	sleep(10);
    }
    unlink($tmpfile);
}

sub start_handler_1
{
    return if shift ne "a";
    my $attr = shift;
    my $href = ${$attr}{"href"};
    #print "$href\n";
    if ($href =~ /^gfs/) {
	my $url = $baseurl . "/" . $href;
	#print "$url\n";
	push @subpaths, $url;
    }
}

sub start_handler_2
{
    return if shift ne "a";
    my $attr = shift;
    my $href = ${$attr}{"href"};
    #print "$href\n";
    if ($href =~ /^gfs.t[0-9]+z.pgrb2f00$/) {
	my $url2 = $url . "/" . $href;
	print "Dataset: $url2\n";
	push @gfsfiles, $url2;
    }
}
