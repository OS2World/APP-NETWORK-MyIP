/*  REXX script to generate a buildlevel string
    creates a buildlevel.txt file or puts into env var which can be linked with
    wlink ... option description @..\buildlevel.txt or
    wlink ... option description %BLDLEVEL_ENVVAR

    For use with MyIP. Uses enviroment variable %hostname% as build
    machine entry. Date/Time/Version is extracted from bldlevel.obj to match
    program output

    20100815 AB initial

*/

VendorName = 'AB'       /* no more than 31 chars */
BuildMachine = value('hostname',,'OS2ENVIRONMENT')
ASD      = ''
Language = ''         /* f.i. EN or empty string */
Country  = ''           /* f.i. AT or empty string */
CPU      = ''           /* U for uniprocessor or empty string */
FixPack  = ''           /* */
Description = 'Get "real world" IP address' /* no more than 79 characters */

/* process command line parameters
called with trace or t gives trace (debug) messages
called with TRACE or T gives even more debug info   */
fTrace = 0
IF LEFT(ARG(1), 1) = 't' THEN fTrace = 1
IF LEFT(ARG(1), 1) = 'T' THEN fTrace = 2
/* SAY LEFT(ARG(1), 1) */

/* limit vendor name */
IF LENGTH(VendorName) > 31 THEN DO
    VendorName = LEFT(VendorName,31)
END

/* get version info from bldlevel.h */
file = 'bldlevel.h'
IF fTrace > 0 THEN SAY 'Lines='lines(file)
DO WHILE lines(file) > 0
    line = LINEIN(file)

    pos = WORDPOS('VER_MAJOR' , line)
    IF pos > 0 THEN DO
        ver_maj = WORD(line, pos + 1)
    END

    pos = WORDPOS('VER_MINOR' , line)
    IF pos > 0 THEN DO
        ver_min = WORD(line, pos + 1)
    END

    pos = WORDPOS('VER_BUILD' , line)
    IF pos > 0 THEN DO
        ver_build = WORD(line, pos + 1)
    END
    IF LENGTH(ver_build) > 7 THEN ver_build = LEFT(ver_build,7)

END

IF fTrace > 0 THEN SAY 'Major='ver_maj' Minor='ver_min' Build='ver_build

/* extract compile date/time from bldlevel.obj */
file = 'bldlevel.obj'
SearchText = 'BuildDateTime'
SearchTextOffs = LENGTH(SearchText) + 1
DO WHILE lines(file) > 0
    line = LINEIN(file)
    pos = POS(SearchText , line)
    IF pos > 0 THEN DO
        pos = pos + SearchTextOffs
        line = SUBSTR(line, pos,40)  /* max. 20 characters for DEBUG string */
        IF fTrace > 0 THEN SAY 'Date='line
        month = WORD(line, 1)
        day = WORD(line, 2)
        year = WORD(line, 3)
        time = WORD(line, 4)
        month=WORDPOS(month, 'Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec')
        IF LENGTH(month) < 2 THEN month = INSERT('0',month)
        IF LENGTH(day) < 2 THEN day = INSERT('0',day)
        IF fTrace > 1 THEN SAY 'Tag='day' Monat='month' Jahr='year
        /* check if it's a debug build */
        IF POS('DEBUG_BUILD', line) > 0 THEN DO
            IF fTrace > 0 THEN SAY 'Debug build'
            ver_min=ver_min'.DEBUG'
            END
    END
END

/* process time */
time = TRANSLATE(time, ' ', ':')
hour = WORD(time, 1)
minu=  WORD(time, 2)
IF LENGTH(hour) < 2 THEN hour = INSERT('0',hour)
IF LENGTH(minu) < 2 THEN minu = INSERT('0',minu)
time = hour':'minu

datetime = year'-'month'-'day' 'time
IF fTrace > 1 THEN SAY 'datetime='datetime

/* compose Feature string (:ASD:EN:US:4b:U:8101) */
Feature=':'ASD':'Language':'Country':'ver_build':'CPU':'FixPack
/* SAY 'Feature='Feature */

/* build time stamp und build machine string */
/* date/time have to be 26 chars (leading ' ' required) */
IF LENGTH(BuildMachine) > 11 THEN DO
    BuildMachine = LEFT(BuildMachine, 11)
    END
bldmachlen = LENGTH(BuildMachine)
IF fTrace > 1 THEN SAY 'bldmach='BuildMachine'<-- len='bldmachlen
datimmach = LEFT(datetime, 24)
datimmach = datimmach' 'BuildMachine
IF fTrace > 1 THEN SAY datimmach'<--'

/* bldlevel do not display more than 79 characters */
IF LENGTH(Description) > 79 THEN DO
    Description = LEFT(Description, 79)
    SAY 'Description lenght corrected'
    END

/* vacbld do have a bug with bldlevel information when added via env var (does not honor string end)
    curiousely it ignores trailing 0 and works well when filled up with 0s (max. length 255 - LENGTH(Description))
    some spaces at the end seem to work too */
/*Description = INSERT(' ' ,Description, LENGTH(Description), 79 - LENGTH(Description), ' ')
  */

FullString = '@#'VendorName':'ver_maj'.'ver_min'#@##1## 'datimmach''Feature'@@  'Description
rc = value('BLDLEVEL_ENVVAR', FullString,'OS2ENVIRONMENT')

FullString ="'"FullString"'"
'@CALL del bldlevel.txt 2>NUL >NUL'
file = 'bldlevel.txt'
IF LINEOUT(file,FullString,1) <> 0 THEN SAY "ERROR - can't write buildlevel.txt"
ELSE SAY FullString
/*SAY FullString*/

