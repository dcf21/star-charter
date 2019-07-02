VII/229  New Catalog of Optically Visible Open Clusters and Candidates  (Dias+ 2002)
====================================================================================
New catalog of optically visible open clusters and candidates
    Dias W.S., Alessi B.S., Moitinho A., Lepine J.R.D.
   <Astron. Astrophys. 389, 871 (2002)>
   =2002A&A...389..871D
====================================================================================
ADC_Keywords: Clusters, open ; Proper motions ; Radial velocities ;
              Abundances, [Fe/H]
Keywords: galaxy: open clusters and associations: general - catalogs

  
Description:
    We have compiled a new catalogue of open clusters in the Galaxy which updates the previous
    catalogues of Lynga (1987) and of Mermilliod (1995) (included in the WEBDA database). New
    objects and new data, in particular, data on kinematics (proper motions) that were not present
    in the old catalogues, have been included. Virtually all the clusters (1629) presently known
    were included, which represents an increment of about 476 objects relative to the Lynga (1987)
    catalogue. The catalogue is presented in a single table containing all the important data,
    which makes it easy to use.

    In total, 99% of the objects have estimates of their apparent diameters, and 38% have distance,
    E(B-V) and age determinations. Concerning the data on kinematics, 22% have their mean proper
    motions listed, 14% their mean radial velocities, and 11% have both information simultaneously.


File Summary:
-------------------------------------------------------------------------------------------
 FileName   Lrecl  Records    Explanations
-------------------------------------------------------------------------------------------
× ReadMe            80        .   This file
× clusters.dat     188     1637   The Catalogue Data File (tables 1a and 1b)
× refs.dat          79       30   References for proper motions and radial velocities
× sources.dat      179       79   The sources used for the catalogue (table 2)
× phot.dat          44      569   Available photometric data on Jan 23 2002 (table 3)
-------------------------------------------------------------------------------------------

See also:
    VII/92 : Open Cluster Data 5th Edition (Lynga 1987)
   VII/101 : Star Clusters/Associations. III. Open Clusters (Ruprecht+ 1983)
   http://www.astro.iag.usp.br/~wilton : Open clusters and Galactic structure
   http://obswww.unige.ch/webda     : WEBDA database





Byte-by-byte Description of file:  clusters.txt
-------------------------------------------------------------------------------------------------
   Bytes Format Units   Label     Explanations
-------------------------------------------------------------------------------------------------
   1- 18  A18   ---     Name      Cluster name
  19- 20  I2    h       RAh       Right ascension (J2000.0)
  22- 23  I2    min     RAm       Right ascension (J2000.0)
  25- 26  I2    s       RAs       Right ascension (J2000.0)
      29  A1    ---     DE-       Declination sign (J2000.0)
  30- 31  I2    deg     DEd       Declination (J2000.0)
  33- 34  I2    arcmin  DEm       Declination (J2000.0)
  36- 37  I2    arcsec  DEs       Declination (J2000.0)
  41- 43  A3    ---     class     [*] Flag for classification of the cluster
  46- 52  F6.1  arcmin  Diam      Apparent diameter in arcmin
  56- 60  I5    pc      d         ? Distance from WEBDA (3)
  66- 70  F5.3          EBV       Colour excess in BV
  74- 79  F6.3  yr      Age       Age (in log t)  
      85  A1    ---     pmRA-	  Mean proper motion of the cluster mu_alpha sign
  86- 90  F5.2  mas/yr  pmRACl    Mean proper motion of the cluster in mu_alpha.cos(delta), ICRS
  93- 96  F4.2  mas/yr  e_pmRACl  Standard deviation in pmRA, ICRS
     101  A1    ---     pmDE-     Mean proper motion of the cluster in mu_delta sign
 102-106  F5.2  mas/yr  pmDECl    Mean proper motion of the cluster in mu_delta, ICRS
 110-113  F4.2  mas/yr  e_pmDECl  Standard deviation in pmRA and pmDE, ICRS
 115-117  I3    ---     Nc        Estimated number of members in the cluster
 120-123  I3    ---     ref1      Source of the mean proper motion determination 
     128  A1    ---     RV-       Radial Velocity sign 
 129-134  F6.2  km/s    RV        Radial Velocity 
 139-143  F5.1  km/s    eRV       Error in Radial Velocity
 149-150  I2    ---     N         Number of stars used to determine Radial Velocity 
 156-159  I4    ---     ref2      Source of the mean radial velocity determination
     163  A1    ---     ME-       Metallicity sign
 164-168  F4.2  index   ME        Metallicity
 172-175  F4.2  index   eME       Error in Metallicity    
 179-180  I2    ---     Nme       Number of stars used to determine Metallicity
 184-191  A8    ---     TrTyp     Trumpler Type determined in the DSS inspection
-------------------------------------------------------------------------------------------------

Byte-by-byte Description of file:  clustersGAL.txt
-------------------------------------------------------------------------------------------------
   Bytes Format Units   Label     Explanations
-------------------------------------------------------------------------------------------------
   1- 18  A18   ---     Name      Cluster name
  19- 26  F8.4  deg     GLON      Galactic longitude II (J2000.0)
      33  A1    ---     DE-       Galactic latitude sign (J2000.0)
  34- 40  F7.4  deg     GLAT      Galactic latitude II (J2000.0)
  47- 48  A2    ---     class     [*] Flag for classification of the cluster
  54- 58  F5.1  arcmin  Diam      Apparent diameter in arcmin
  62- 66  I4    pc      d         ? Distance from WEBDA (3)
  72- 76  F5.3          EBV       Colour excess in BV
  81- 85  F5.3  yr      Age       Age (in log t)  
      93  A1    ---     pml- 	  Mean proper motion of the cluster mu_l sign
  94- 98  F5.2  mas/yr  pml       Mean proper motion of the cluster in mul, ICRS
 102-105  F3.2  mas/yr  e_pml     Standard deviation in pml, ICRS
     108  A1    ---     pmb-      Mean proper motion of the cluster in mu_b sign
 109-113  F5.2  mas/yr  pmDECl    Mean proper motion of the cluster in mu_b, ICRS
 119-122  F3.2  mas/yr  e_pmRACl  Standard deviation in pmb, ICRS
 124-126  I3    ---     Nc        Estimated number of members in the cluster
 130-132  I3    ---     ref1      Source of the mean proper motion determination 
     138  A1    ---     RV-       Radial Velocity sign 
 139-143  F5.2  km/s    RV        Radial Velocity 
 149-152  F4.1  km/s    eRV       Error in Radial Velocity
 159-160  I2    ---     N         Number of stars used to determine Radial Velocity 
 166-169  I4    ---     ref2      Source of the mean radial velocity determination
     173  A1    ---     ME-       Metallicity sign
 174-178  F4.2  index   ME        Metallicity
 182-185  F4.2  index   eME       Error in Metallicity    
 189-190  I2    ---     Nme       Number of stars used to determine Metallicity
 194-201  A8    ---     TrTyp     Trumpler Type determined in the DSS inspection
-------------------------------------------------------------------------------------------------

Notes on class: The flag denotes:
    a     possible asterism/dust hole/star cloud ( no cluster )
    e     embedded open cluster (or cluster associated with nebulosity)
    g     possible globular cluster
    m     possible moving group
    n     "non-existent NGC" ( RNGC, Sulentic 1979). Some of Bica's POCRs are also "non-existent NGC" objects.
    o     possible OB association ( or detached part of )
    p     POCR (Possible Open Cluster Remnant) - Bica et al. 2001, A&A 366, 827
    v     clusters with variable extinction (Ahumada et al. 2001, A&A...377..845A)
    r     recovered: "non-existent NGC" that are well visible in the DSS images inspection
    d     dubious: objects considered doubtful by the DSS images inspection
    nf    not found: objects not found in the DSS images inspection (wrog coordinates?)
    cr    Cluster Remnant (Pavani D., & Bica E., 2007, A&A 468, 139)
    OC?   possible cluster (Kronberger M, private communication)
    OC    likely cluster   (Kronberger M, private communication)
    IR    discovered in infra-red but are visible in the DSS images inspection
    
    
Notes on ref1 and ref2: The flag denotes:
        
    ALE     Alessi B. S., 2003, private communication
    AMD     Alessi B. S., Moitinho A., Dias W. S., 2003, A&A 410, 565
    BDM     Balog Z., Delgado, A.J., Moitinho A.,et al. 2001, MNRAS 323, 872
    BDW     Baumgardt H., Dettbarn C., Wielen R., 2000, A&AS 146, 251 
    BMB     Beauchamp A., Moffat A. F.J. and Drissen L., 1994, ApJS 93, 187
    BMTB    Boeche, C., Munari, U., Tomasella, L.and Barbon, R., 2004, A&A 415,145
    CCP     Carraro G., Chaboyer B., and Perencevich J., 2005, astro-ph/0510573
    CGVF    Carraro, G., Geisler D., Villanova S., Frinchaboy, P. M.,, and  Majewski S. R., 2008, arXiv:0709.2126v1
    CM92    Clariá J.J., Mermilliod J.-C. : 1992, Astron. Astrophys. Suppl. 95, 429 
    CMP     Clariá J.J., Mermilliod J.-C., Piatti A.E.. : 1999, Astron. Astrophys. Suppl. 134, 301 
    CMPM    Clariá J.J., Mermilliod J.-C., Piatti A.E., Minniti D. : 1994, Astron. Astrophys. Suppl. 107, 39 
    CMVB    Carraro G., et al., 2008, arXiv:astro-ph/0701758v1
    CPML    Clariá J.J., Piatti A.E., Lapasset E., Mermilliod J.-C., 2003 A&A 399, 543
    CVD     Carraro, G., Villanova, S., Demarque, P., et al. 2008, ArXiv:0802.3243v1
    Car     Carraro G., 2002, in press in A&A (astro-ph/0203156)
    D06     Dias, W. S., Assafin M., Flório V., et al. 2006, A&A in press
    DL03    Dias & Lépine, 2003, submitted
    DMA     Delgado A.J., Miranda L.F., Alfaro E.J. : 1999, AStron. J. 118, 1759 
    DMFA    Delgado A. J., Miranda L. F., Fernandez M., Alfaro E. J., 2004, AJ 128, 330
    DMFA    Delgado A. J., Miranda L. F., Fernandez M., Alfaro E. J., 2004, AJ 128, 330
    EMCA    Eigenbrod A., Mermilliod J.-C., Clariá J. J, et al., 2004, A&A 423, 189
    ESLK    Evans C. J., Smartt, S. J., Lee, J.-K, 2005, A&A 437, 467
    F89     Friel E.D. : 1993, PASP 101, 244 
    FJ02    Friel E.D., Janes K. A., Tavares M., et. al. 2002, AJ 124, 2693
    FJ93    Friel E.D., Janes K.A. : 1993, Astron. Astrophys. 267, 75 
    FJP     Friel E. D., et al. 2005, AJ 129, 2725
    FM09    Frinchaboy, P. M., Majewski, S. R., 2008, AJ 136, 118F
    FMMF    Frinchaboy P. M., Muñoz, R. R., Majewski S. R. et al., astro-ph/0411127
    GO01    Gonzalez and Lapasset, 2001,AJ 121,2657
    GR99    Grenier et al. 1999, A&AS 135, 503
    H87     Hron 1987, A&A 176, 34
    HGAM    Hole, K. T., et al., 2009, AJ 138, 159H
    HRHW    Hunsch M., et al. 2004, A&A 418, 539
    K05     Kharchenko, N. V.; Piskunov, A. E.; Röser, S.; Schilbach, E.; Scholz, R.-D., 2005, A&A 438, 1163 
    K07     Kharchenko, N. V., et al. 2007, AN 328,.889K
    KBVM    Carraro G., et al. 2004, AJ 128, 1676
    KHA     Kharchenko, N. V.; Pakulyak, L. K.; Piskunov, A. E., 2003ARep...47..263K
    LD91    Latham, David W.; et al. 1991, AJ.101..625L
    LI89    Liu, T., Janes, K., A. and Bania, T. M. 1989 AJ 98, 626
    LI91    Liu, T., Janes, K. A. and Bania, T. M. 1991 AJ 102, 1103
    LOK     Loktin, A. V.and Beshenov, G. V., 2003ARep...47....6L
    M09     Mermilliod, J.-C., M. Mayor, and S. Udry, 2009, A&A 498, 949
    MAD     Moitinho, A.; Alessi, B. S.; Dias, W. S., 2003EAS....10..141M
    MAM     Mamajek E., 2005, private communication
    MANM    Mermilliod J.-C., Andersen J., Nordström B., Mayor M. : 1995, Astron. Astrophys. 299, 53 
    MCAM    Mermilliod J.-C., Clariá J.J., Andersen J., Mayor M : 1997, Astron. Astrophys. 324, 91 
    MCAP    Mermilliod J.-C., Clariá J.J., Andersen J., Piatti A. E. and Mayor M., 2001 Astron. Astrophys.375, 30
    MHRM    Mermilliod J.-C., Hustamendia G. del Rio G., Mayor M. : 1996, Astron. Astrophys. 307, 80 
    MLGI    Mermilliod J-C, et al. 2003, A&A 399, 105
    MM89    Mermilliod J.-C., Mayor M. : 1989, Astron. Astrophys. 219, 125 
    MM90    Mermilliod J.-C., Mayor M. : 1990, Astron. Astrophys. 237, 61 
    MMB     Mermilliod J.-C., Mayor M., Burki G. : 1987, Astron. Astrophys. Suppl. 70, 389 
    MMLM    Mermilliod J.-C., Mathieu R.D., Latham D.W., Mayor M. : 1998, Astron. Astrophys. 339, 423 
    MMU     Mermilliod, J. C.; Mayor, M.; Udry, S.2008, A&A 485, 303
    MRWP    Manzi, S., Randich S., de Wit W.J.,  and  Palla F., 2008, arXiv:0712.0226v1
    PCA     Piatti, A. E.; Clariá, J. J., Ahumada, A. V., 2009, MNRAS 397, 1073 
    PLA     Platais I., Kozhurina-Platais V., van Leeuwen F. : 1998, Astron J. 116, 2423
    PMFK    Platais, I., et al. 2008MNRAS.391.1482P
    RB99    Robichon, N., Arenou, F., Mermilliod, J.-C., Turon, C., 1999, A&A 345, 471
    RGDZ    Rastorguev A.S., Glushkova E.V., Dambis A.K. & Zabolotskikh M.V. : 1999, Astron. Letters 25, 689 
    RM98    Raboud D., Mermilliod J.-C. : 1998, Astron. Astrophys. 329, 101 
    RPPB    Randich, S.; Pace, G.; Pastori, L.; Bragaglia, A., 2009, A&A 496, 441
    SFJ     Scott J.E., Friel E.D., Janes K.A. : 1995, Astron. J. 109, 1706
    SOC     Soubiran C., Oudnkirchen M and Le Campion J-F, 2000, astro-ph 0003148
    ST44    Struve O. 1944, ApJ 100, 189
    TFV     Thorgensen E.N., Friel E.D., Fallon B.V. : 1993, PASP 105, 1253 
    TYC     Dias, Lépine and Alessi 2002, A&A 388, 168 (d>1kpc)
    TYC     Dias, Lépine and Alessi 2001, A&A 376, 441 (d<1kpc)
    VB      Vázques and Baume, 2001, A&A 371, 908
    VBC     Villanova S., Baume G., Carraro G. and Geminale A., 2004 astroph/0401492
    VBC2    Villanova S., Baume G., Carraro G. 2007, ASTROPH 0705.2300
    VGB     Villanova S., Carraro G., Bresolin F., 2005, astro-ph/0504282
    VR10    Villanova, S., et al. 2010, A&A 509, 102 
    WC09    Warren, Steven R.; Cole, Andrew A., 2009, MNRAS.393, .272
    YCA     Young D., Carney B. W., Almeida M. L. T., 2005, astro-ph/0504193

    


Byte-by-byte Description of file:  refs.dat
--------------------------------------------------------------------------------
   Bytes Format Units   Label     Explanations
--------------------------------------------------------------------------------
   1-  4  A4    --     Ref       Reference code
   6- 24  A19   --     BibCode   Bibcode of the references
  26- 48  A23   --     Aut       Author's name
  50- 79  A30   --     Com       Comments
--------------------------------------------------------------------------------

Byte-by-byte Description of file:  sources.dat
--------------------------------------------------------------------------------
   Bytes Format Units   Label     Explanations
--------------------------------------------------------------------------------
   1- 21  A21   --     Object    Objects associated with the reference
  23- 41  A19   --     BibCode   BibCode of the reference
  43- 66  A24   --     Aut       Author's name
  67-179  A113  --     Com       Comments
--------------------------------------------------------------------------------

Byte-by-byte Description of file:  phot.dat
--------------------------------------------------------------------------------
   Bytes Format Units  Label    Explanations
--------------------------------------------------------------------------------
   1- 17  A17   --    Cluster  Cluster name
  19- 23  A5    --    CCD      [UBVRI ] Bands observed with CCD
  25- 29  A5    --    Pe       [UBVRI ] Bands observed with photomultipliers
  31- 33  A3    --    Pg       [UBV ] Bands observed with photographic plates
  35- 54  A20   --    Alias    Other name
--------------------------------------------------------------------------------

Global notes:

Note (G1): The flag denotes:
    a: possible asterism/dust hole/star cloud (no cluster)
    d: dubious, objects considered doubtful by the DSS images inspection
    e: embedded open cluster (or cluster associated with nebulosity)
    g: possible globular cluster
   IR: originally detected in Infra-Red, but visible in optical DSS images inspection
    m: possible moving group
    n: "non-existent NGC" (RNGC, Sulentic, 1979, Cat. VII/1). Some of
       Bica's POCRs (Possible Open Cluster Remnant, 2001A&A...366..827B)
       are also "non-existent NGC" objects.
   nf: objects not found in the DSS images inspection (wrong coordinates?)
    o: possible OB association (or detached part of)
    p: POCR (Possible Open Cluster Remnant) - Bica et al., 2001A&A...366..827B
    r: recovered, i.e. "non-existent NGC" that are well visible
       in the DSS images inspection
    v: clusters with variable extinction (Ahumada et al., 2001A&A...377..845A)
  Deleted clusters (Globular, not Open Clusters) are:
  ESO 280-06, ESO 452-11, ESO 456-38, IC 1257, Ruprecht 106, BH 176, Lynga 7
--------------------------------------------------------------------------------

History:

    Version of 20-apr-2010 at http://www.astro.iag.usp.br/~wilton (version 3.0)
    Version of 04-feb-2009 at http://www.astro.iag.usp.br/~wilton (version 2.10)
    Version of 13-apr-2008 at http://www.astro.iag.usp.br/~wilton (version 2.9)
    Version of 17-sep-2007 at http://www.astro.iag.usp.br/~wilton (version 2.8)
    Version of 27-oct-2006 at http://www.astro.iag.usp.br/~wilton (version 2.7)
    Version of 11-jan-2006 at http://www.astro.iag.usp.br/~wilton (version 2.6)
    Version of 03-oct-2005 at http://www.astro.iag.usp.br/~wilton (version 2.5)
    Version of 09-sep-2005 at http://www.astro.iag.usp.br/~wilton (version 2.4)
    Version of 25-apr-2005 at http://www.astro.iag.usp.br/~wilton (version 2.3)
    Version of 02-mar-2005 at http://www.astro.iag.usp.br/~wilton (version 2.2)
    Version of 06-jan-2005 at http://www.astro.iag.usp.br/~wilton (version 2.1)
    Version of 07-jun-2004 at http://www.astro.iag.usp.br/~wilton (version 2.0)
    Version of 15-feb-2004 at http://www.astro.iag.usp.br/~wilton (version 1.9)
    Version of 10-Set-2003 at http://www.astro.iag.usp.br/~wilton (version 1.8)
    Version of 03-Set-2003 at http://www.astro.iag.usp.br/~wilton (version 1.7)
    Version of 03-Jul-2003 at http://www.astro.iag.usp.br/~wilton (version 1.6)
    Version of 01-Jun-2003 at http://www.astro.iag.usp.br/~wilton (version 1.5)
    Version of 08-Aug-2002 at http://www.astro.iag.usp.br/~wilton
  * 19-Sep-2002: Galactic coordinates and proper motions recomputed
       from the equatorial position

Acknowledgements:
   Extensive use has been made of the Simbad and WEBDA databases.
   This project is supported by FAPESP (grant number 03/12813-4) and CAPES (CAPES-GRICES grant number 040/2008).
================================================================================
(End)                    Francois Ochsenbein, Patricia Bauer [CDS]   20-Apr-2010


