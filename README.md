Ultimate Bitmap Font Generator
------------------------------

Ultimate bitmap font generator - useful tool to create compact font textures for games

If you know Russian - welcome: http://www.gamedev.ru/projects/forum/?id=152527

Feature list:
* multiple fonts may be packed into one texture
* extremely effective packing algorithm + subcharacters merging
* optimizied not only for English: generate Russian, Japanese, Chinese fonts
* easy-to-create characters list from text
* distance fields for easy OpenGL scalable fonts
* exporting kerning pairs
* free and open-source project

UBFG can export font into XML format (image in base64 format also stored in XML) or into it's own .fnt format, - ready to use with Cheetah 2D engine. XML format is self-described, .fnt format uses following spec.

UBFG generates two files: font.png and font.fnt. 

  - font.png is an image, that represents your font texture. You must load it as texture.
  - font.fnt is a text file, that contains information about all chars on font.png. 

This information looks like:

	Arial 9pt -- font name and size
	Char  X pos   Y pos   Width   Height   Xoffset  Yoffset  Orig W   Orig H
	32    0       0       0       0        3        14       3        14
	97    90      36      5       7        1        4        7        14
	98    0       41      5       9        1        2        7        14
	kerning pairs:
	32 97 -1
	32 98 -0.5


Here:

* Char - UNICODE number of your char (codepage varies and may be specified before export). For example, 32 is a 'space'
* X pos - x position of glyph on texture
* Y pos - y position of glyph on texture
* Width - width of glyph on texture (glyphs are cropped and Width and Orig Width aren't equal)
* Height - height of glyph on texture
* Xoffset - distance on the x-axis, on which glyph must be shifted
* Yoffset - distance on the y-axis, on which glyph must be shifted
* Orig W - original width of glyph
* Orig H - original height of glyph

Also fnt format may have _kerning pairs:_ a set of kerning pairs listed after phrase "kerning pairs:". 
First number if a first character in pair, second number - second character in pair, third number - 
kerning value. Note: this value may be with floating point if you use distance fields.

![Help image](https://github.com/scriptum/UBFG/raw/master/readme.png)

Screenshot:

![Screenshot](https://github.com/scriptum/UBFG/raw/master/screenshot.png)
