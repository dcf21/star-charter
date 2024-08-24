VI/49       Constellation Boundary Data    (Davenhall+ 1989)
================================================================================
Catalogue of Constellation Boundary Data
    Davenhall A.C., Leggett S.K.
    <Royal Obs. Edinburgh (1989)>
================================================================================
ADC_Keywords: Constellations

Description:
    A computer readable catalog of constellation boundary data is
    presented in a form suitable for the construction of star charts and
    atlases. Two data files are available, one for equator and equinox
    1875 and the other for equator and equinox 2000. In addition to the
    data files a documentation file is available that includes a table
    listing the abbreviations used for the constellations as well as a
    more detailed discussion of the preparation of the catalog.

    The present catalog of constellation boundary data is complementary to
    that of Roman (1987). Roman's catalog should be used to determine in
    which constellation an object lies in. The present catalog is more
    suited to the construction of star charts and atlases. Both catalogs
    were based on Delporte (1930).

File Summary:
--------------------------------------------------------------------------------
 FileName    Lrecl  Records   Explanations
--------------------------------------------------------------------------------
ReadMe          80        .   This file
constell.txt    84      260   Plain text document describing the original files
constell.tex    86      299   LaTeX document describing the original files
adc.sty         78       68   adc style file
bound_18.dat    25     1565   Boundaries for B1875 (original file)
bound_20.dat    29    13039   Boundaries for J2000
constbnd.dat    28     1651  *Boundaries for B1875 with adjacent constellations
constbnd.txt    75      174  *Detailed explanations of file "constbnd.dat"
--------------------------------------------------------------------------------
Note on constbnd.dat, constbnd.txt: prepared in 2007 by Bill J. Gray
     (Project Pluto, Bowdoinham Maine, USA).
--------------------------------------------------------------------------------

See also:
    VI/42 : Identification of a Constellation From Position (Roman 1987)
    http://www.iau.org/public/constellations/ : IAU page about constellations
            (International Astronomical Union)

Byte-by-byte Description of file: bound_18.dat
--------------------------------------------------------------------------------
   Bytes  Format   Units   Label   Explanations
--------------------------------------------------------------------------------
   1-  8   F8.5    h       RAhr    Right ascension in decimal hours (B1875)
  10- 18   F9.5    deg     DEdeg   Declination in degrees (1875)
  20- 23   A4      ---     cst     Constellation abbreviation
      25   A1      ---     type    [O] Type of point (O = original)
--------------------------------------------------------------------------------

Byte-by-byte Description of file: bound_20.dat
--------------------------------------------------------------------------------
   Bytes  Format   Units   Label   Explanations
--------------------------------------------------------------------------------
   1- 10   F10.7   h       RAhr    Right ascension in decimal hours (J2000)
  12- 22   F11.7   deg     DEdeg   Declination in degrees (J2000)
  24- 27   A4      ---     cst     Constellation abbreviation
      29   A1      ---     type    [OI] Type of point (Original or Interpolated)
--------------------------------------------------------------------------------

Byte-by-byte Description of file: constbnd.dat
--------------------------------------------------------------------------------
   Bytes  Format   Units   Label   Explanations
--------------------------------------------------------------------------------
   1-  8   F8.5    h       RAhr    Right ascension in decimal hours (B1875)
  10- 18   F9.5    deg     DEdeg   Declination in degrees (1875)
  20- 23   A4      ---     cst     Constellation abbreviation (1)
  25- 28   A4      ---     adj     Adjacent constellation abbreviation (1)
--------------------------------------------------------------------------------
Note (1): The origin point of a constellation has a blank "adj" name, and
     the successive boundary lines follow in a counter-clockwise order.
     See the detailed explanations in the "constbnd.txt" file.
--------------------------------------------------------------------------------

Acknowledgements:
    The authors of the catalog are grateful to Mr. D.A. Pickup for useful
    discussions about the format of Delporte's lists and the effect of
    precession on the shape of the constellation boundaries and to Dr.
    W.H. Warren Jr. for several useful suggestions. This work was carried
    out as part of a contract with the Edinburgh publishers John
    Bartholomew and Son Ltd. to provide data for a revised edition of
    `Norton's Star Atlas'.

References:
   Delporte, E. 1930, Delimitation Scientifique des Constellations
      (Cambridge: Cambridge University Press).
   Roman, N.G. 1987, Publ. Astron. Soc. Pac. 99, pp695-699.

History:
  * 27-May-1997: documentation of the catalog by N.G. Roman (ADC)
  * 07-Apr-2009: one of the boundaries for Oph fixed.
       Files "constbnd.dat" and "constbnd.txt", prepared
       by Bill J. Gray, were added.
  * 14-Jun-2013: Removed the spurious point in Cepheus at (1h+88deg)
  * 22-Aug-2019: J2000 boundaries for UMI and OCT fixed
       (for OCT, previously 758 points and now 376 points)
================================================================================
(End)                    N. G. Roman   [SSDOO/ADC]                   27-May-1997
