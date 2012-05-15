Ultimate Bitmap Font Generator
------------------------------

Ultimate bitmap font generator - useful tool to create compact font textures for games

If you know Russian - welcome: http://www.gamedev.ru/projects/forum/?id=152527

Watch me if you want to see changes:)

UBFG can export font into XML format (image in base64 format also stored in XML) or into it's own .fnt format, - ready to use with Cheetah 2D engine. XML format is self-described, .fnt format uses following spec.

UBFG generates two files: font.png and font.fnt. 

  - font.png is an image, that represents your font texture. Yoou must load it as texture.
  - font.fnt if a text file, that contains information about all glyphs in texture font.png. 

This information looks like:

	Arial 9pt -- font name and size
	Char  X pos   Y pos   Width   Height   Xoffset  Yoffset  Orig W   Orig H
	32    0       0       0       0        3        14       3        14
	97    90      36      5       7        1        4        7        14
	98    0       41      5       9        1        2        7        14


Here:

* Char - ASCII number of your char (CP 1251 codepage). For example, 32 is a 'space'
* X pos - x position of glyph on texture
* Y pos - y position of glyph on texture
* Width - width of glyph on texture (glyphs are cropped and Width and Orig Width aren't equal)
* Height - height of glyph on texture
* Xoffset - distance on the x-axis, on which glyph must be shifted
* Yoffset - distance on the y-axis, on which glyph must be shifted
* Orig W - original width of glyph
* Orig H - original height of glyph

![Help image](https://github.com/scriptum/UBFG/raw/master/readme.png)

Screenshot:

![Screenshot](https://github.com/scriptum/UBFG/raw/master/screenshot.png)