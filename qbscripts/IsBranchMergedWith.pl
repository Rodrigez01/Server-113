#!/usr/bin/perl
# ============================================================================
# IsBranchMergedWith.pl - Branch Merge Checker
# ============================================================================
# ZWECK: Prüft ob ein Source-Branch in einen Destination-Branch gemerged wurde
#
# VORAUSSETZUNGEN:
#   - Perl (Standard-Installation)
#   - Git Repository
#
# VERWENDUNG:
#   perl IsBranchMergedWith.pl --source=feature-branch --destination=master
#
# PARAMETER:
#   --source=branch        Branch der geprüft werden soll
#   --destination=branch   Branch in dem gesucht werden soll
#
# RÜCKGABE:
#   "1" = Branch ist gemerged
#   "0" = Branch ist nicht gemerged
#
# BEISPIEL:
#   perl IsBranchMergedWith.pl --source=feature-113 --destination=master
# ============================================================================

#use strict;
use warnings;
use LWP::Simple;
use Getopt::Long;
use Pithub;


my $source;
my $destination;

GetOptions("source=s", => \$source,
           "destination=s", => \$destination);

$output = `git checkout $destination 2>&1 1>nul`;
$output = `git branch --merged`;

if ($output =~ m/$source/)
{
	print "1\n";
}
else
{
	print "0\n";
}