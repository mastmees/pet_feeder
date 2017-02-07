# MIT License
#
# Copyright (c) 2017 Madis Kaal
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

bits = { "a":1,"b":2,"c":4,"d":8,"e":16,"f":32,"g":64 }

# this map tells which of the 7 segments need to be lit
# to display a character
letters = {
  "0":"abcdef",
  "1":"bc",
  "2":"abedg",
  "3":"abcdg",
  "4":"bcfg",
  "5":"acdfg",
  "6":"acdefg",
  "7":"abc",
  "8":"abcdefg",
  "9":"abcdfg",
  "A":"abcefg",
  "b":"cdefg",
  "C":"adef",
  "c":"deg",
  "d":"bcdeg",
  "E":"adefg",
  "F":"aefg",
  "G":"acdef",
  "h":"cefg",
  "i":"c",
  "j":"c",
  "K":"bcefg",
  "L":"def",
  "M":"abcef",
  "n":"ceg",
  "o":"cdeg",
  "P":"abefg",
  "q":"cdeg",
  "r":"eg",
  "S":"acdfg",
  "t":"defg",
  "u":"cde",
  "v":"cde",
  "W":"bcdef",
  "X":"bcefg",
  "y":"bcdfg",
  "z":"abedg",
  "-":"g",
  " ":""  
}

# now just create C code for bitmaps
for c in sorted(letters.keys()):
  print "{ '%c', "%c.upper(),
  l=letters[c]
  cc=0
  for bl in l:
    b=bits[bl]
    cc=cc+b
  print "0x%02x },"%cc
