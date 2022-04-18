#pragma once

#define TAG(name, child_tags) "<" name ">\n" child_tags "</" name ">\n"
#define A_HREF(href, child_tags) "<a href=\"" href "\">\n" child_tags "</a>\n"
#define IMG(src, alt_text) "<img src=\"" src "\" alt=\"" alt_text "\" />\n"
#define LINK(rel, href) "<link rel=\"" rel "\" href=\"" href "\" />\n"
#define UNARY_TAG(name) "<" name " />\n"
#define DOCTYPE(type) "<!DOCTYPE " type ">\n"
#define META(name, http_equiv, content, charset) "<meta name=\"" name "\" http_equiv=\"" http_equiv "\" content=\"" content "\" charset=\"" charset"\" >\n"
#define BASE(href, target) "<base href=\"" href "\" target=\"" target "\" >\n"