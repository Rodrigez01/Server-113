#!/usr/bin/perl
# ============================================================================
# automerge.pl - Simple Build Branch Creator & Merger
# ============================================================================
# ZWECK: Erstellt einen Build-Branch von master und merged Changelists hinein
#
# VORAUSSETZUNGEN:
#   - Perl (Standard-Installation)
#   - Git Repository
#
# VERWENDUNG:
#   perl automerge.pl --build=113 --change=feature1 --change=feature2
#
# PARAMETER:
#   --build=113        Build-Nummer (wird Branch-Name)
#   --change=branch    Branch zum Mergen (mehrfach möglich)
#
# BEISPIEL:
#   perl automerge.pl --build=113 --change=113-feature1 --change=113-feature2
#   # Erstellt Branch "113" von master
#   # Merged "113-feature1" und "113-feature2" hinein
# ============================================================================

use strict;
use warnings;
use LWP::Simple;
use Getopt::Long;

my @changelists;
my $build;

GetOptions ("change=s" => \@changelists,
            "build=s" => \$build);

system("git checkout master");

system("git checkout -b $build master");
	
foreach my $changelist (@changelists)
{
	my $RC = system("git merge $build-$changelist");	
	if ($RC != 0)
	{
		die "Merge of branch $build-$changelist failed";
	}
}