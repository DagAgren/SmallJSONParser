#!/usr/bin/perl

use strict;

if(@ARGV!=2)
{
	print STDERR "Usage: $0 inputsourcecode output.md\n";
	print STDERR "\n";
	print STDERR "//-style comments will be stripped and printed to stdout.\n";
	print STDERR "Any lines immediately following will be printed before the\n";
	print STDERR "comment text, and indented by one tab as a Markdown code block.\n";
	print STDERR "\n";
	print STDERR "If // is not followed by a space, the comment will be ignored.\n";
	exit 1;
}

open INPUT,$ARGV[0] or die "Could not read file $ARGV[0]";
open OUTPUT,">$ARGV[1]" or die "Could not create file $ARGV[1]";

my $lookingforcode=0;
my $lookingforcomments=1;
my @commentlines=();
my @codelines=();

while(<INPUT>)
{
	chomp;

	if($lookingforcomments and m!^// (.*)!)
	{
		push @commentlines,$1;
		$lookingforcode=1;
	}
	elsif($lookingforcomments and m!^//$!)
	{
		push @commentlines,"";
		$lookingforcode=1;
	}
	elsif($lookingforcode)
	{
		if(m!^\s*$!) # All-whitespace line.
		{
			foreach my $line (@codelines) { print OUTPUT "\t$line\n" }
			print OUTPUT "\n" if(@codelines);

			foreach my $line (@commentlines) { print OUTPUT "$line\n" }
			print OUTPUT "\n";

			@codelines=();
			@commentlines=();

			$lookingforcode=0;
			$lookingforcomments=1;
		}
		else
		{
			push @codelines,$_;
			$lookingforcomments=0;
		}
	}
}

if(@commentlines)
{
	foreach my $line (@codelines) { print OUTPUT "\t$line\n" }
	print OUTPUT "\n" if(@codelines);

	foreach my $line (@commentlines) { print OUTPUT "$line\n" }
}
