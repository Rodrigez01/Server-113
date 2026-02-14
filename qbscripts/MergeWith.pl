#!/usr/bin/perl
# ============================================================================
# MergeWith.pl - Simple Git Merge Helper
# ============================================================================
# ZWECK: Merged einen Source-Branch in einen Destination-Branch
#
# VORAUSSETZUNGEN:
#   - Perl (Standard-Installation)
#   - Git Repository
#
# VERWENDUNG:
#   perl MergeWith.pl --source=feature-branch --destination=master
#
# PARAMETER:
#   --source=branch        Branch der gemerged werden soll
#   --destination=branch   Ziel-Branch
#
# BEISPIEL:
#   perl MergeWith.pl --source=feature-113 --destination=master
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

$output = `git checkout $destination`;

$output = `git merge $source`;