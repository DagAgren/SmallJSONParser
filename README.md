# SmallJSONParser #

This is a simple, one-file JSON parser in C. It is designed for highly
resource-constrained systems. It uses no memory allocation, and can stream
data, so that the whole file does not need to reside in memory.

It does not actually convert the JSON structure into a native data structure
or tree. Instead, it works more like a tokeniser, and finds each primitive
value in turn, and also reports when an object or array starts or ends.

The application then follows along with the structure, and extracts the data
it needs as it encounters it. The implementation also includes a set of
helper functions to make this easier.

## Simplifications ##

In order to keep code size down, the parser actually parses a simplified JSON
syntax. Any well-formed JSON data will be parsed correctly by the simplified
grammar, but many kinds of JSON data that a strict parser would not accept will
be accepted by this parser.

* `:` and `,` are ignored entirely.
* `]` and `}` are treated as interchangeable.
* Any bareword of any length starting with `t` is treated as a `true` value.
* Any bareword of any length starting with `f` is treated as a `false` value.
* Any bareword of any length starting with `n` is treated as a `null` value.
* Number parsing may be less strict that the spec requires.

For instance, this can be parsed as valid data: `{"a" t "b" f "c" [1 2 3}}`.

## API documentation ##

The API contains several layers, all quite simple.

### Minimal API ###

At this level, the parser is supplied with a buffer of memory containing JSON
data, and the parsing function is called repeatedly, returning one token per
call. When the end of buffer is reached, the parser may return a partial token
(for instance, if the buffer ends in the middle of a string value).

Once the end of the buffer has been released, a new buffer with the following
data can be supplied, and the parser will carry in where it left off, and return
the remaining data of any partial token it previously returned.

If the whole JSON structure can be fit into one buffer, partial tokens do not
need to be handled.

Value data is returned in tokens simply as start and end pointers into the
original data. You can access this data directly yourself, or you can use the
token parsing API functions, defined further down, to parse string and
number values.

### Pull API ###

To facilitate streaming data through the parser, a slightly higher-level API
is provided on top of the minimal API. With this API, you provide a callback
function that loads and provides data to the parser as needed. You also provide
a memory buffer that is used to reconstruct partial tokens across buffer
boundaries.

If a token needs reconstructed, the start and end pointers of the returned
token will be pointing into the reconstruction buffer instead of into the
original data.

As the reconstruction buffer is of fixed size, if a value is too big to fit
into the provided memory, it is truncated to fit, and any remaining data is
discarded. If this is a problem, you should use the minimal API to handle
reconstructing tokens yourself, or just load the entire JSON struture into
memory so there are no buffer boundaries to deal with.

### Token parsing API ###

These functions help with extracting data from string and number values.
Numbers can be parsed both as integers or floating-point values. Strings can
be processed to expand escape codes, including Unicode escapes which are
converted into UTF-8. String escape expansion an either be done in-place or
into a separate buffer. If it is done in-place, the original data buffer is
modified, so make sure this is possible and acceptable.

### Structure parsing API ###

TODO.

## License ##

This code is released into the public domain with no warranties. If that is not
suitable, it is also available under the
[CC0 license](http://creativecommons.org/publicdomain/zero/1.0/).
