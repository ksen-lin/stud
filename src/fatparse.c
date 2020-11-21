#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
//https://stackoverflow.com/questions/526430/c-programming-how-to-program-for-unicode
#include <uchar.h>
#include <wchar.h>

#define ATTR_RO        0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20
#define ATTR_RESERVED  0x40

#pragma pack(push, 1)
struct biospb {
    short sector_size;
    char  sectincluster;
    short ressects;    // 0x0e
    char  fat_count;
    short root_fd_num;       // 0x11
    short sectors;
    char  media;
    short fat_sectors;  // 0x16
    short track_sectors; // 0x18
    short heads;
    int   hiddensects;
    int   largesects;
    char  phdisc_num;
    char  curr_head; // not used
    char  sig;
    int   serial; // volume serial
    char  label[11];
    char  systemid[8];
};

// fat32 only
struct fsinfo {

};

struct fat12bs {
    char  jmp[3];
    char  id[8];
    struct biospb bpb;
    struct biospb ebpb;
    int info;
};

struct long_filename {
    char LDIR_ord;
    char16_t name1[5];
    char LDIR_attr;
    char LDIR_type;
    char LDIR_chksum;

    char16_t name2[6];
    short LDIR_first_clus_lo;
    char16_t name3[2];
};

struct file_record {
    char name[11];
    char attr;
    char nt;
    char crt_time10th;
    short crt_time;
    short crt_date;
    short last_acc;
    short fst_clus_hi;
    short wr_time;
    short wr_date;
    short fst_clus_lo;
    int   fsize;
};


// fat16
// 0000: free,
// 0002-ffef: cluster in use; the value given is the number of the next cluster in the file,
// fff0-fff6: reserved,
// fff7: bad cluster,
// fff8-ffff: cluster in use, the last one in this file
//  Now the maximum possible size of a FAT16 volume is 2 GB (65526 clusters of at most 64 sectors each).

#pragma pack(pop)

struct options {
    char verbose;
};

struct slack_space {
    int offset;
    short length;
};


/* * * * * * * * * * * * * *
 *      G L O B A L S      *
 * * * * * * * * * * * * * */
struct slack_space slack[10];
struct options opts = {0};
struct fat12bs bootsect;
FILE  *f = (void *)0;
short *fat = (void *)0;
int    fat_n = 0;
short   slack_n = 0;



void print_lfn(struct long_filename *addr)
{
    int i;
    for (i = 0; i < 5; i += 1)
        putchar(addr->name1[i]);
    for (i = 0; i < 6; i += 1)
        putchar(addr->name2[i]);
    for (i = 0; i < 2; i += 1)
        putchar(addr->name3[i]);
    return;
}



void show_slacks()
{
    int i;
    printf(" [*] %i SLACK SPACES FOUND:\n", slack_n);
    for (i = 0; i < slack_n; i += 1) {
        printf(" %2i: offset  %#-8x\n"
               "     length  %#4hx (%3$hu)\n", i, slack[i].offset, slack[i].length);
    }
}


void write2slacks()
{
    int choice, off;
    char *buf;
    printf("choose space to write to [0-%i]: ", slack_n - 1);
    scanf("%d%*c", &choice);
    if (choice >= slack_n) {
        printf("wrong choice...\n");
        return;
    }

    freopen((char*)0, "r+", f);
    perror("freopen");
    off = slack[choice].offset + bootsect.bpb.sector_size *
             bootsect.bpb.sectincluster - slack[choice].length;
    fseek(f, off, 0);
    buf = malloc(slack[choice].length);

    printf("enter your data: ");
    fgets(buf, slack[choice].length, stdin);
     /* removing LF */
    buf[strlen(buf) - 1] = '\0';
    printf("writing `%s' to %#x...\n\n", buf, off);
    fwrite(buf, 1, strlen(buf), f);
    perror("fwrite");
    free(buf);
}



short last_clus(short clus)
{
    if (!clus)
        return 0;

    while ((short)0xffff != fat[clus]) {
        clus = fat[clus];
    }
    printf(" [+] found last cluster: %#hx\n", clus);
    return clus;
}


void show_clus_chain(short clus)
{
    if (!opts.verbose || !clus)
        return;

    printf("   clusters:\n");
    while (clus != (short)0xffff && clus != 0) {
        printf("%#x -> ", clus);
        clus = fat[clus];
    }
    printf("%#6hx [EOF]\n", clus);
}


void print_file(struct file_record *frec,
                short cluster_size,
                short sec_per_clus,
                int fst_data_sect)
{
    int clus_no   = ((frec->fst_clus_hi << 16) | frec->fst_clus_lo),
        base_sect = sec_per_clus * (clus_no-2) + fst_data_sect,
        tmp;
    printf("  |  name:  \"%.11s\"\n", frec->name);
    printf("  |  size: %i bytes\n",  frec->fsize);
    printf("  |  start:  offset %#8x  sector %#x (clus %#x)\n",
           base_sect*cluster_size/sec_per_clus, base_sect, clus_no);

    show_clus_chain(clus_no);

    if (frec->fsize % cluster_size) {
        tmp = last_clus(clus_no);
        slack[slack_n].offset = cluster_size/sec_per_clus*((tmp-2)*sec_per_clus + fst_data_sect);
        slack[slack_n].length = cluster_size - frec->fsize % cluster_size;
        printf(" ~~~ %u bytes available in the trailing block ~~~\n"
               "   you can find it in cluster %#x (offset %#x)\n\n",
                slack[slack_n].length, tmp, slack[slack_n].offset);
        slack_n += 1;
    }
}

#define ENT_NUM bootsect.bpb.sector_size/sizeof(struct file_record)
/* assert: dir takes only one cluster */
void parse_dir(struct file_record *dir, int fst_data_sect, int n)
{
    if (n++ > 5)
        return;
    // TODO: `tabs' to show recursion depth
    char sp[20];
    struct file_record entries[ENT_NUM];
    int i, clus_no   = ((dir->fst_clus_hi << 16) | dir->fst_clus_lo);
//     memset(sp, ' ', 4*n);
    sp[0] = '\0';

    printf(" [>] ENTERING LEVEL %i\n", n);
    show_clus_chain(clus_no);
    i = (bootsect.bpb.sectincluster * (clus_no-2) + fst_data_sect) *
           bootsect.bpb.sector_size;

    printf(" [*] seeking to %#x...\n", i);
    fseek(f, i, 0);
    fread(entries, 1, sizeof(entries), f);

    // first 2 entries are self and root, so skip them
    for (i = 2; i < 8; i += 1) {
        if ((0xff & entries[i].name[0]) == 0xe5 ||
            (0xff & entries[i].name[0]) == 0x05)
        {
            printf("%s [*] record marked deleted\n", sp);
            continue;
        } else if ((0xff & entries[i].name[0]) == 0) {
            printf("%s [*] record and the rest of directory marked deleted\n", sp);
        } else {
            printf("%s [+] record ok\n", sp);
        }
        printf("%s  |  fst_clus: %#x\n", sp, (entries[i].fst_clus_lo));

        switch (entries[i].attr) {
        case (ATTR_VOLUME_ID | ATTR_ARCHIVE):
            printf("%s  |  found volume label record\n", sp);
            printf("%s  |  name: \"%.11s\"\n", sp, entries[i].name);
            break;

        case (ATTR_RO | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID):
            if (opts.verbose) {
                printf("%s  |  lfn (ord %#x)\n  |  \"", sp,
                       0xff&((struct long_filename *)(&entries[i]))->LDIR_ord);
                print_lfn((struct long_filename *)&entries[i]);
                printf("\"\n");
            }
            break;

        case ATTR_DIRECTORY:
            printf("%s  |  directory\n", sp);
            printf("%s  |  name: \"%.11s\"\n", sp, entries[i].name);
            parse_dir(&entries[i], fst_data_sect, n);
            break;

        default:
            print_file(&entries[i],
                       bootsect.bpb.sector_size*bootsect.bpb.sectincluster,
                       bootsect.bpb.sectincluster,
                       fst_data_sect);
            break;
        }
    }
    printf("\n [<] LEAVING LEVEL %i\n", n);
    return;
}


int main(int argc, char *argv[])
{
    if (argc > 1)
        opts.verbose = 1;

    errno = 0;
    int i, tmp;
    int first_data_sector, root_dir_clus, root_dir_sectors;

    f = fopen("adams.dd", "r");
    perror("fopen");

    /* reading FAT16 boot sector */
    fread(&bootsect, 1, sizeof(struct fat12bs), f);

    printf(" ~~~ G E N E R A L   F S   I N F O ~~~\n");
    printf("                 id: \"%.4s\"\n", bootsect.id);
    printf("        sector_size: %i\n", bootsect.bpb.sector_size);
    printf("sectors per cluster: %i\n", bootsect.bpb.sectincluster);
    printf("        fat_sectors: %i\n", (int)bootsect.bpb.fat_sectors);
    printf("               FATs: %i\n", (int)bootsect.bpb.fat_count);
    printf("        root_fd_num: %i\n", bootsect.bpb.root_fd_num);
    printf("      volume serial: %#x\n", bootsect.bpb.serial);
    printf("       volume label: \"%.11s\"\n", bootsect.bpb.label);
    printf("          system_id: \"%.8s\"\n", bootsect.bpb.systemid);
    printf("             fsinfo: %#x\n", bootsect.info);
    printf("   root_dir_sectors: %i\n", root_dir_sectors =
           32*bootsect.bpb.root_fd_num/bootsect.bpb.sector_size);

    /* reading 1st FAT */
    fseek(f, 512, 0);
    tmp = bootsect.bpb.fat_sectors * bootsect.bpb.sector_size;
    fat_n = tmp/sizeof(short);
    fat = malloc(tmp);
    fread(fat, 1, tmp, f);

    /* reading root directory*/
    root_dir_clus = 1 + 2*bootsect.bpb.fat_sectors;
    printf(" root_dir at sector: %i (offset %#x)\n", root_dir_clus, root_dir_clus*512);
    fseek(f, root_dir_clus*512, 0);

    first_data_sector = bootsect.bpb.ressects +
                        bootsect.bpb.fat_count*bootsect.bpb.fat_sectors +
                        root_dir_sectors;
    printf(" first_data_sector : %i (offset %#x)\n", first_data_sector, first_data_sector*512);

    struct file_record root_dir[bootsect.bpb.root_fd_num-1];
    fread(root_dir, 1, sizeof(root_dir), f);


    /* Набор LFN-записей каталога FAT всегда должен быть
     * связан с обычной SFN-записью                      */
    for (i = 0; i < 20 /*bootsect.bpb.root_fd_num/2*/; i += 1) {
        putchar('\n');
        if ((0xff & root_dir[i].name[0]) == 0xe5 ||
            (0xff & root_dir[i].name[0]) == 0x05)
        {
            printf(" [*] record marked deleted\n");
            continue;
        } else if ((0xff & root_dir[i].name[0]) == 0) {
            printf(" [*] record and the rest of directory marked deleted\n");
        } else {
            printf(" [+] record ok\n");
        }
        printf("  |  fst_clus: %#x\n", (root_dir[i].fst_clus_lo));

        switch (root_dir[i].attr) {
        case (ATTR_VOLUME_ID | ATTR_ARCHIVE):
            printf("  |  found volume label record\n");
            printf("  |  name: \"%.11s\"\n", root_dir[i].name);
            break;

        case (ATTR_RO | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID):
            if (opts.verbose) {
                printf("  |  lfn (ord %#x)\n  |  \"",
                       0xff&((struct long_filename *)(&root_dir[i]))->LDIR_ord);
                print_lfn((struct long_filename *)&root_dir[i]);
                printf("\"\n");
            }
            break;

        case ATTR_DIRECTORY:
            printf("  |  directory\n");
            printf("  |  name: \"%.11s\"\n", root_dir[i].name);
            parse_dir(&root_dir[i], first_data_sector, 0);
            break;

        default:
            print_file(&root_dir[i],
                       bootsect.bpb.sector_size*bootsect.bpb.sectincluster,
                       bootsect.bpb.sectincluster,
                       first_data_sector);
            break;
        }
    }

    show_slacks();
    write2slacks();

    errno = 0;
    free(fat);
    fclose(f);
    perror("fclose");
    return 0;
}