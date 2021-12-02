#include "video_detect.h"

#include <string.h>

#include "util.h"

/*
 * Base Media File Format "Box"
 * As defined by the ISO/IEC 14496-12:2015 standard
 * [https://www.iso.org/standard/68960.html]
 * - A size of 1 means the size is actually in the field 'largesize' (???)
 * - A size of 0 means this is the last box, and its contents extends to EOF
 */
struct BMFF_Box {
    u32 size;       // Big-endian
    char type[4];   // ASCII
};

/*
 * We use ftyp, major brand and version simply to determine if we are
 * looking at an mp4 file.
 * This info can further be used to know what specification(s) the file
 * complies with, but we don't care for our purposes.
 */
struct BMFF_Filetype_Box {
    struct BMFF_Box box;
    char major_brand[4];            // ASCII
    u32 minor_version;              // Big-endian
    char compatible_brands[][4];    // ASCII: 4 bytes per brand
};

/*
 * The file contains one presentation metadata wrapper: the Movie Box.
 * Usually this is close to the beginning or end of the file.
 * Meta-data Box(es) are contained inside the Movie Box.
 * Other boxes found at this level may be File-Type box, Free-Space boxes,
 * Movie Fragments and Media Data boxes.
 */

/*
 * Return:
 *   1: File is an mp4
 *   0: File is not an mp4 or does not exist
 */
int is_mp4(char *filepath)
{
    // [see: ./_REF/mp4-layout.txt : https://xhelmboyx.tripod.com/formats/mp4-layout.txt]
    // TODO: Make sure we have enough to recognise an mp4
    struct BMFF_Filetype_Box header;

    {
        FILE *f = fopen(filepath, "r");
        if (!f)
            return 0;
        u32 amount_read = fread(&header, 1, sizeof(header), f);
        fclose(f);
        // Files smaller than the header are clearly not .mp4's
        if (amount_read < sizeof(header))
            return 0;
    }

    header.box.size = endswap32(header.box.size);
    header.minor_version = endswap32(header.minor_version);

    if (strncmp(header.box.type, "ftyp", 4) != 0) {
        return 0;
    }

    // NOTE: Any brand less than 4 chars is padded with spaces, as per the spec
    char *brands[] = {"isom", "iso2", "mp41", "mp42", "qt  ", "avc1", "mmp4", "mp71"};

    u8 match = 0;
    for (int i = 0, len = sizeof(brands) / sizeof(*brands); i < len; i++) {
        if (strncmp(header.major_brand, brands[i], 4) == 0) {
            match = 1;
            break;
        }
    }

    //printf("[DEBUG] Offset: %d\n", header.offset);
    //printf("[DEBUG] ftyp: %.4s\n", header.ftyp);
    //printf("[DEBUG] Major Brand: %.4s\n", header.major_brand);
    //printf("[DEBUG] Major Brand Version: %d\n", header.minor_version);

    return match;
}
