#!/usr/bin/env perl

use MaxMind::DB::Writer::Tree;

my $tree = MaxMind::DB::Writer::Tree->new(
    ip_version            => 4,
    record_size           => 24,
    remove_reserved_networks => 0,
    database_type         => 'libvmod-geoip2',
    description           => { en => 'libvmod-geoip2 test database' },
    map_key_type_callback => sub { 'utf8_string' },
);

$tree->insert_network(
    '127.0.0.1/8',
    {
        long_string => 'thequickbrownfoxjumpsoverthelazydog1234567890',
    },
);

open my $fh, '>:raw', 'libvmod-geoip2.mmdb';
$tree->write_tree($fh);
close $fh;
