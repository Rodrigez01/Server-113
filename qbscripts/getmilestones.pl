#!/usr/bin/perl
# ============================================================================
# getmilestones.pl - GitHub Milestones Lister
# ============================================================================
# ZWECK: Listet alle GitHub Milestones eines Repositories
#
# VORAUSSETZUNGEN:
#   - Perl Module: cpan install Pithub LWP::Simple
#
# VERWENDUNG:
#   perl getmilestones.pl
#
# KONFIGURATION FÜR SERVER 113/114:
#   Zeile 21-22: GitHub User/Repo ändern
# ============================================================================

use strict;
use warnings;
use LWP::Simple;
use Getopt::Long;
use Data::Dumper;
use Pithub;

my $m = Pithub::Issues::Milestones->new;

# ========== KONFIGURATION: GitHub Repository ==========
# TODO: Ändere 'OpenMeridian105' zu deinem GitHub User
# TODO: Ändere 'Meridian59' zu deinem Repository Name
my $result = $m->list(
    repo  => 'Meridian59',          # ← HIER ÄNDERN
    user  => 'OpenMeridian105',     # ← HIER ÄNDERN
);
# ========================================================


while ( my $row = $result->next ) 
{
	printf "%s:%s\n", $row->{number},$row->{title};
}