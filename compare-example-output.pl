#! /usr/bin/env perl

use v5.14;    # implicit use strict, use feature ':5.14'
use warnings FATAL => 'all';
use utf8;
use open qw(:utf8);
no  if $] >= 5.022, warnings => 'experimental::re_strict';
use if $] >= 5.022, re       => 'strict';

sub filter_log {
    my ($ifname, $ofname) = @_;
    open my $ifp, '<', $ifname
        or die "$ifname: $!\n";
    open my $ofp, '>', $ofname
        or die "$ofname: $!\n";

    while (my $l = <$ifp>) {
        chomp $l;
        $l =~ s/\s+$//;
        $l =~ s/\.c(?:c|pp)?:\K\d+/nn/g;
        $l =~ s/\(\d+ ticks, \d+\.\d+ sec\)/(nn ticks, n.nnn sec)/g;
        $l =~ s/, seed \d+$/, seed nnnnn/;
        $l =~ s/init_second_pass: a \d+, c \d+, state \d+/init_second_pass: <<variable>>/;

        print $ofp $l, "\n";
    }
    close $ifp;
    close $ofp or die "$ofname: write error: $!\n";
}

sub main {
    eval {
        filter_log('example-output.log', 'example-output-filtered.log');
        exec 'diff', '-u', 'example-output.exp', 'example-output-filtered.log';
        die "diff: $!\n";
    };

    # we only ever get here on error
    print {*STDERR} "$@";
    exit 1;
}

main();
