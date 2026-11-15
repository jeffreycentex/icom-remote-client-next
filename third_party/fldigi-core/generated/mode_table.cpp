#include "web_compat.h"
const struct mode_info_t mode_info[NUM_MODES] = {

{MODE_NULL,nullptr,"NULL","NULL","","NULL","","","", 0 },

{MODE_CW,nullptr,"CW","CW","CW","CW","CW","","CW", 0 },

{MODE_CONTESTIA,nullptr,"CONTESTIA","Contestia","","CONTESTI","CONTESTI","","CT",0},

{MODE_CONTESTIA_4_125,nullptr,"Cont-4/125","Cont4-125","","CONTESTI4/125","CONTESTI","","CT4/125",0},
{MODE_CONTESTIA_4_250,nullptr,"Cont-4/250","Cont4-250","","CONTESTI4/250","CONTESTI","","CT4/250",0},
{MODE_CONTESTIA_4_500,nullptr,"Cont-4/500","Cont4-500","","CONTESTI4/500","CONTESTI","","CT4/500",0},
{MODE_CONTESTIA_4_1000,nullptr,"Cont-4/1K","Cont4-1K","","CONTESTI4/1000","CONTESTI","","CT4/1K",0},
{MODE_CONTESTIA_4_2000,nullptr,"Cont-4/2K","Cont4-2K","","CONTESTI4/2000","CONTESTI","","CT4/2K",0},

{MODE_CONTESTIA_8_125,nullptr,"Cont-8/125","Cont8-125","","CONTESTI8/125","CONTESTI","","CT8/125",0},
{MODE_CONTESTIA_8_250,nullptr,"Cont-8/250","Cont8-250","","CONTESTI8/250","CONTESTI","","CT8/250",0},
{MODE_CONTESTIA_8_500,nullptr,"Cont-8/500","Cont8-500","","CONTESTI8/500","CONTESTI","","CT8/500",0},
{MODE_CONTESTIA_8_1000,nullptr,"Cont-8/1K","Cont8-1K","","CONTESTI8/1000","CONTESTI","","CT8/1K",0},
{MODE_CONTESTIA_8_2000,nullptr,"Cont-8/2K","Cont8-2K","","CONTESTI8/2000","CONTESTI","","CT8/2K",0},

{MODE_CONTESTIA_16_250,nullptr,"Cont-16/250","Cont16-250","","CONTESTI16/250","CONTESTI","","CT16/250",0},
{MODE_CONTESTIA_16_500,nullptr,"Cont-16/500","Cont16-500","","CONTESTI16/500","CONTESTI","","CT16/500",0},
{MODE_CONTESTIA_16_1000,nullptr,"Cont-16/1K","Cont16-1K","","CONTESTI16/1000","CONTESTI","","CT16/1K",0},
{MODE_CONTESTIA_16_2000,nullptr,"Cont-16/2K","Cont16-2K","","CONTESTI16/2000","CONTESTI","","CT16/2K",0},

{MODE_CONTESTIA_32_1000,nullptr,"Cont-32/1K","Cont32-1K","","CONTESTI32/1000","CONTESTI","","CT32/1K",0},
{MODE_CONTESTIA_32_2000,nullptr,"Cont-32/2K","Cont32-2K","","CONTESTI32/2000","CONTESTI","","CT32/2K",0},

{MODE_CONTESTIA_64_500,nullptr,"Cont-64/500","Cont64-500","","CONTESTI64/500","CONTESTI","","CT64/500",0},
{MODE_CONTESTIA_64_1000,nullptr,"Cont-64/1K","Cont64-1K","","CONTESTI64/1000","CONTESTI","","CT64/1K",0},
{MODE_CONTESTIA_64_2000,nullptr,"Cont-64/2K","Cont64-2K","","CONTESTI64/2000","CONTESTI","","CT64/2K",0},

{MODE_DOMINOEXMICRO,nullptr,"DOMEXM","DominoEX Micro","DOMINOEXMICRO","DOMINO","DOMINO","","DM M", 0 },
{MODE_DOMINOEX4,nullptr,"DOMEX4","DominoEX 4","DOMINOEX4","DOMINO","DOMINO","","DM 4", 0 },
{MODE_DOMINOEX5,nullptr,"DOMEX5","DominoEX 5","DOMINOEX5","DOMINO","DOMINO","","DM 5", 0 },
{MODE_DOMINOEX8,nullptr,"DOMEX8","DominoEX 8","DOMINOEX8","DOMINO","DOMINO","","DM 8", 0 },
{MODE_DOMINOEX11,nullptr,"DOMX11","DominoEX 11","DOMINOEX11","DOMINO","DOMINO","","DM11", 0 },
{MODE_DOMINOEX16,nullptr,"DOMX16","DominoEX 16","DOMINOEX16","DOMINO","DOMINO","","DM16", 0 | 0 },
{MODE_DOMINOEX22,nullptr,"DOMX22","DominoEX 22","DOMINOEX22","DOMINO","DOMINO","","DM22", 0 | 0 },
{MODE_DOMINOEX44,nullptr,"DOMX44","DominoEX 44","DOMINOEX44","DOMINO","DOMINO","","DM44", 0 | 0 },
{MODE_DOMINOEX88,nullptr,"DOMX88","DominoEX 88","DOMINOEX88","DOMINO","DOMINO","","DM88", 0 | 0 },

{MODE_FELDHELL, nullptr,     "FELDHELL", "Feld Hell",    "", "HELL", "HELL", "",         "HELL",  0 },
{MODE_SLOWHELL, nullptr, "SLOWHELL", "Slow Hell",    "", "HELL", "HELL", "SLOWHELL", "SHLL",  0 },
{MODE_HELLX5,   nullptr,   "HELLX5",   "Feld Hell X5", "", "HELL", "HELL", "HELLX5",   "HLX5",  0 },
{MODE_HELLX9,   nullptr,   "HELLX9",   "Feld Hell X9", "", "HELL", "HELL", "HELLX9",   "HLX9",  0 },
{MODE_FSKH245,  nullptr,   "FSKH245",  "FSK Hell-245", "", "HELL", "HELL", "FSKH245",  "FSKHL", 0 },
{MODE_FSKH105,  nullptr,"FSKH105",  "FSK Hell-105", "", "HELL", "HELL", "FSKH105",  "H105",  0 },
{MODE_HELL80,   nullptr,   "HELL80",   "Hell 80",      "", "HELL", "HELL", "HELL80",   "HL80",  0 },

{MODE_MFSK8,nullptr,"MFSK8","MFSK-8","MFSK8","MFSK8","MFSK","MFSK8","MK 8", 0  },
{MODE_MFSK16,nullptr,"MFSK16","MFSK-16","MFSK16","MFSK16","MFSK","MFSK16","MK16", 0 | 0 },
{MODE_MFSK32,nullptr,"MFSK32","MFSK-32","MFSK32","MFSK32","MFSK","MFSK32","MK32", 0 | 0  },

{MODE_MFSK4,nullptr,"MFSK4","MFSK-4","MFSK4","MFSK4","MFSK","MFSK4","MK 4", 0  },
{MODE_MFSK11,nullptr,"MFSK11","MFSK-11","MFSK11","MFSK11","MFSK","MFSK11","MK11", 0  },
{MODE_MFSK22,nullptr,"MFSK22","MFSK-22","MFSK22","MFSK22","MFSK","MFSK22","MK22", 0  },
{MODE_MFSK31,nullptr,"MFSK31","MFSK-31","MFSK31","MFSK31","MFSK","MFSK31","MK31", 0 | 0 },
{MODE_MFSK64,nullptr,"MFSK64","MFSK-64","MFSK64","MFSK64","MFSK","MFSK64","MK64", 0 | 0   },
{MODE_MFSK128,nullptr,"MFSK128","MFSK-128","MFSK128","MFSK128","MFSK","MFSK128","MK128", 0 | 0   },
{MODE_MFSK64L,nullptr,"MFSK64L","MFSK-64L","MFSK64L","MFSK64L","MFSK","MFSK64","MK64L", 0 | 0   },
{MODE_MFSK128L,nullptr,"MFSK128L","MFSK-128L","MFSK128L","MFSK128L","MFSK","MFSK128","MK128L", 0 | 0  },

{MODE_WEFAX_576,nullptr,"WEFAX576","WEFAX-IOC576","WEFAXIOC576","FAX","FAX","","FX576", 0 },
{MODE_WEFAX_288,nullptr,"WEFAX288","WEFAX-IOC288","WEFAXIOC288","FAX","FAX","","FX288", 0 },

{MODE_NAVTEX,nullptr,"NAVTEX","NAVTEX","NAVTEX","TOR","NAVTEX","","NAVTEX", 0 },
{MODE_SITORB,nullptr,"SITORB","SITORB","SITORB","TOR","SITORB","","SITORB", 0 },

{MODE_MT63_500S,nullptr,"MT63-500S","MT63-500S","MT63-500S","MT63-500S","MT63","","MT63-500S", 0 | 0  },
{MODE_MT63_500L,nullptr,"MT63-500L","MT63-500L","MT63-500L","MT63-500L","MT63","","MT63-500L", 0 | 0  },
{MODE_MT63_1000S,nullptr,"MT63-1KS","MT63-1000S","MT63-1XXS","MT63-1KS","MT63","","MT63 1KS", 0 | 0  },
{MODE_MT63_1000L,nullptr,"MT63-1KL","MT63-1000L","MT63-1XXL","MT63-1KL","MT63","","MT63 1KL", 0 | 0  },
{MODE_MT63_2000S,nullptr,"MT63-2KS","MT63-2000S","MT63-2XXS","MT63-2KS","MT63","","MT63 2KS", 0 | 0  },
{MODE_MT63_2000L,nullptr,"MT63-2KL","MT63-2000L","MT63-2XXL","MT63-2KL","MT63","","MT63 2KL", 0 | 0  },

{MODE_PSK31,nullptr,"BPSK31","BPSK-31","PSK31","PSK31","PSK","PSK31","P31", 0 | 0  },
{MODE_PSK63,nullptr,"BPSK63","BPSK-63","PSK63","PSK63","PSK","PSK63","P63", 0 | 0  },
{MODE_PSK63F,nullptr,"BPSK63F","BPSK-63F","PSK63F","PSK63F","PSK","PSK63F","P63F", 0  | 0 },
{MODE_PSK125,nullptr,"BPSK125","BPSK-125","PSK125","PSK125","PSK","PSK125","P125",  0 | 0  },
{MODE_PSK250,nullptr,"BPSK250","BPSK-250","PSK250","PSK250","PSK","PSK250","P250",  0 | 0  },
{MODE_PSK500,nullptr,"BPSK500","BPSK-500","PSK500","PSK500","PSK","PSK500","P500",  0 | 0  },
{MODE_PSK1000,nullptr,"BPSK1000","BPSK-1000","PSK1000","PSK1000","PSK","PSK1000","P1000",  0 | 0  },

{MODE_12X_PSK125,nullptr,"PSK125C12","12xPSK125","PSK125C12","PSK125C12","PSK","","P125C12", 0 | 0 },
{MODE_6X_PSK250,nullptr,"PSK250C6","6xPSK250","PSK250C6","PSK250C6","PSK","","P2506", 0 | 0 },
{MODE_2X_PSK500,nullptr,"PSK500C2","2xPSK500","PSK500C2","PSK500C2","PSK","","2xP500", 0 | 0 },
{MODE_4X_PSK500,nullptr,"PSK500C4","4xPSK500","PSK500C4","PSK500C4","PSK","","4xP500", 0 | 0 },
{MODE_2X_PSK800,nullptr,"PSK800C2","2xPSK800","PSK800C2","PSK800C2","PSK","","P800RC2", 0 | 0 },
{MODE_2X_PSK1000,nullptr,"PSK1000C2","2xPSK1000","PSK1000C2","PSK1000C2","PSK","","P1KC2", 0 | 0 },

{MODE_QPSK31,nullptr,"QPSK31","QPSK-31","QPSK31","QPSK31","PSK","QPSK31","Q31", 0  | 0   },
{MODE_QPSK63,nullptr,"QPSK63","QPSK-63","QPSK63","QPSK63","PSK","QPSK63","Q63", 0  | 0   },
{MODE_QPSK125,nullptr,"QPSK125","QPSK-125","QPSK125","QPSK125","PSK","QPSK125","Q125",  0 | 0  },
{MODE_QPSK250,nullptr,"QPSK250","QPSK-250","QPSK250","QPSK250","PSK","QPSK250","Q250",  0 | 0  },
{MODE_QPSK500,nullptr,"QPSK500","QPSK-500","QPSK500","QPSK500","PSK","QPSK500","Q500",  0 | 0  },

{MODE_8PSK125,nullptr,"8PSK125","8PSK-125","8PSK125","8PSK125","PSK","","8PSK125", 0 | 0 },
{MODE_8PSK125FL,nullptr,"8PSK125FL","8PSK-125FL","8PSK125FL","8PSK125FL","PSK","","8PSK125FL", 0 | 0 },
{MODE_8PSK125F,nullptr,"8PSK125F","8PSK-125F","8PSK125F","8PSK125F","PSK","","8PSK125F", 0 | 0 },
{MODE_8PSK250,nullptr,"8PSK250","8PSK-250","8PSK250","8PSK250","PSK","","8PSK250", 0 | 0 },
{MODE_8PSK250FL,nullptr,"8PSK250FL","8PSK-250FL","8PSK250FL","8PSK250F","PSK","","8PSK250FL", 0 | 0 },
{MODE_8PSK250F,nullptr,"8PSK250F","8PSK-250F","8PSK250F","8PSK250F","PSK","","8PSK250F", 0 | 0 },
{MODE_8PSK500,nullptr,"8PSK500","8PSK-500","8PSK500","8PSK500","PSK","","8PSK500", 0 | 0 },
{MODE_8PSK500F,nullptr,"8PSK500F","8PSK-500F","8PSK500F","8PSK500F","PSK","","8PSK500F", 0 | 0 },
{MODE_8PSK1000,nullptr,"8PSK1000","8PSK-1000","8PSK1000","8PSK1000","PSK","","8PSK1000", 0 | 0 },
{MODE_8PSK1000F,nullptr,"8PSK1000F","8PSK-1000F","8PSK1000F","8PSK1000F","PSK","","8PSK1000F", 0 | 0 },
{MODE_8PSK1200F,nullptr,"8PSK1200F","8PSK-1200F","8PSK1200F","8PSK1200F","PSK","","8PSK1200F", 0 | 0 },

{MODE_OFDM_500F,nullptr,"OFDM500F","OFDM-500F","OFDM500F","OFDM500F","OFDM","","OFDM500F", 0 | 0 },
{MODE_OFDM_750F,nullptr,"OFDM750F","OFDM-750F","OFDM750F","OFDM750F","OFDM","","OFDM750F", 0 | 0 },


{MODE_OFDM_3500,nullptr,"OFDM3500","OFDM-3500","OFDM3500","OFDM3500","OFDM","","OFDM3500", 0 | 0 },

{MODE_OLIVIA,nullptr,"OLIVIA","OLIVIA","OLIVIA","OLIVIA","OLIVIA","","OL", 0 },

{MODE_OLIVIA_4_125,nullptr,"OLIVIA-4/125","OL 4-125","OLIV 4-125","OLIVIA 4/125","OLIVIA","OLIVIA 4/125","OL4/125", 0  },
{MODE_OLIVIA_4_250,nullptr,"OLIVIA-4/250","OL 4-250","OLIV 4-250","OLIVIA 4/250","OLIVIA","OLIVIA 4/250","OL4/250", 0  },
{MODE_OLIVIA_4_500,nullptr,"OLIVIA-4/500","OL 4-500","OLIV 4-500","OLIVIA 4/500","OLIVIA","OLIVIA 4/500","OL4/500", 0 | 0 },
{MODE_OLIVIA_4_1000,nullptr,"OLIVIA-4/1K","OL 4-1K","OLIV 4-1K","OLIVIA 4/1K","OLIVIA","OLIVIA 4/1K","OL4/1K", 0 | 0 },
{MODE_OLIVIA_4_2000,nullptr,"OLIVIA-4/2K","OL 4-2K","OLIV 4-2K","OLIVIA 4/2K","OLIVIA","OLIVIA 4/2K","OL4/2K", 0 | 0 },

{MODE_OLIVIA_8_125,nullptr,"OLIVIA-8/125","OL 8-125","OLIV 8-125","OLIVIA 8/125","OLIVIA","OLIVIA 8/125","OL8/125", 0  },
{MODE_OLIVIA_8_250,nullptr,"OLIVIA-8/250","OL 8-250","OLIV 8-250","OLIVIA 8/250","OLIVIA","OLIVIA 8/250","OL8/250", 0  },
{MODE_OLIVIA_8_500,nullptr,"OLIVIA-8/500","OL 8-500","OLIV 8-500","OLIVIA 8/500","OLIVIA","OLIVIA 8/500","OL8/500", 0 | 0 },
{MODE_OLIVIA_8_1000,nullptr,"OLIVIA-8/1K","OL 8-1K","OLIV 8-1K","OLIVIA 8/1K","OLIVIA","OLIVIA 8/1K","OL8/1K", 0 | 0  },
{MODE_OLIVIA_8_2000,nullptr,"OLIVIA-8/2K","OL 8-2K","OLIV 8-2K","OLIVIA 8/2K","OLIVIA","OLIVIA 8/2K","OL8/1K", 0 | 0  },

{MODE_OLIVIA_16_500,nullptr,"OLIVIA-16/500","OL 16-500","OLIV 16-500","OLIVIA 16/500","OLIVIA","OLIVIA 16/500","OL16/500", 0 | 0 },
{MODE_OLIVIA_16_1000,nullptr,"OLIVIA-16/1K","OL 16-1K","OLIV 16-1K","OLIVIA 16/1K","OLIVIA","OLIVIA 16/1K","OL16/1K", 0 | 0  },
{MODE_OLIVIA_16_2000,nullptr,"OLIVIA-16/2K","OL 16-2K","OLIV 16-2K","OLIVIA 16/2K","OLIVIA","OLIVIA 16/2K","OL16/2K", 0 | 0  },

{MODE_OLIVIA_32_1000,nullptr,"OLIVIA-32/1K","OL 32-1K","OLIV 32-1K","OLIVIA 32/1K","OLIVIA","OLIVIA 32/1K","OL32/1K", 0 | 0  },
{MODE_OLIVIA_32_2000,nullptr,"OLIVIA-32/2K","OL 32-2K","OLIV 32-2K","OLIVIA 32/2K","OLIVIA","OLIVIA 32/2K","OL32/2K", 0 | 0  },

{MODE_OLIVIA_64_500,nullptr,"OLIVIA-64/500","OL 64-500","OLIV 64-500","OLIVIA 64/500","OLIVIA","","OL64/500", 0 | 0  },
{MODE_OLIVIA_64_1000,nullptr,"OLIVIA-64/1K","OL 64-1K","OLIV 64-1K","OLIVIA 64/1K","OLIVIA","","OL64/1K", 0 | 0  },
{MODE_OLIVIA_64_2000,nullptr,"OLIVIA-64/2K","OL 64-2K","OLIV 64-2K","OLIVIA 64/2K","OLIVIA","","OL64/2K", 0 | 0  },

{MODE_RTTY,nullptr,"RTTY","RTTY","RTTY","RTTY","RTTY","","RY", 0 },

{MODE_SCAMPFSK,nullptr,"SCAMPFSK","SCAMPFSK","SCAMPFSK","MFSK", "MFSK", "SCAMPFSK", "SCFSK", 0 },
{MODE_SCAMPOOK,nullptr,"SCAMPOOK","SCAMPOOK","SCAMPOOK","MFSK", "MFSK", "SCAMPOOK", "SCOOK", 0 },
{MODE_SCFSKFST,nullptr,"SCFSKFST","SCFSKFST","SCFSKFST","MFSK", "MFSK", "SCFSKFST", "SFFST", 0 },
{MODE_SCFSKSLW,nullptr,"SCFSKSLW","SCFSKSLW","SCFSKSLW","MFSK", "MFSK", "SCFSKSLW", "SFSLW", 0 },
{MODE_SCOOKSLW,nullptr,"SCOOKSLW","SCOOKSLW","SCOOKSLW","MFSK", "MFSK", "SCOOKSLW", "SOSLW", 0 },
{MODE_SCFSKVSL,nullptr,"SCFSKVSL","SCFSKVSL","SCFSKVSL","MFSK", "MFSK", "SCFSKVSL", "SFVSL", 0 },

{MODE_THORMICRO,nullptr,"THORM","THOR Micro","THORM","THOR-M","THOR","","THM", 0  },
{MODE_THOR4,nullptr,"THOR4","THOR 4","THOR4","THOR4","THOR","","TH4", 0  },
{MODE_THOR5,nullptr,"THOR5","THOR 5","THOR5","THOR5","THOR","","TH5", 0  },
{MODE_THOR8,nullptr,"THOR8","THOR 8","THOR8","THOR8","THOR","","TH8", 0  },
{MODE_THOR11,nullptr,"THOR11","THOR 11","THOR11","THOR11","THOR","","TH11", 0  },
{MODE_THOR16,nullptr,"THOR16","THOR 16","THOR16","THOR16","THOR","","TH16", 0 | 0 },
{MODE_THOR22,nullptr,"THOR22","THOR 22","THOR22","THOR22","THOR","","TH22", 0 | 0 },

{MODE_THOR25,nullptr,"THOR25","THOR 25","THOR25","THOR25","THOR","","TH25", 0 | 0 },
{MODE_THOR32,nullptr,"THOR32","THOR 32","THOR32","THOR32","THOR","","TH32", 0 | 0 },
{MODE_THOR44,nullptr,"THOR44","THOR 44","THOR44","THOR44","THOR","","TH44", 0 | 0 },
{MODE_THOR56,nullptr,"THOR56","THOR 56","THOR56","THOR56","THOR","","TH56", 0 | 0 },

{MODE_THOR25x4,nullptr,"THOR25x4","THOR 25 x4","THOR25x4","THOR-25X4","THOR","","TH25", 0 | 0 },
{MODE_THOR50x1,nullptr,"THOR50x1","THOR 50 x1","THOR50x1","THOR-50X1","THOR","","TH51", 0 | 0 },
{MODE_THOR50x2,nullptr,"THOR50x2","THOR 50 x2","THOR50x2","THOR-50X2","THOR","","TH52", 0 | 0 },
{MODE_THOR100,nullptr,"THOR100","THOR 100","THOR100","THOR-100","THOR","","TH100", 0 | 0 },

{MODE_THROB1,nullptr,"THROB1","Throb 1","","THRB","THRB","","TB1", 0 },
{MODE_THROB2,nullptr,"THROB2","Throb 2","","THRB","THRB","","TB2", 0 },
{MODE_THROB4,nullptr,"THROB4","Throb 4","","THRB","THRB","","TB4", 0 },
{MODE_THROBX1,nullptr,"THRBX1","ThrobX 1","","THRBX","THRB","THRBX","TX1", 0 },
{MODE_THROBX2,nullptr,"THRBX2","ThrobX 2","","THRBX","THRB","THRBX","TX2", 0 },
{MODE_THROBX4,nullptr,"THRBX4","ThrobX 4","","THRBX","THRB","THRBX","TX4", 0 },



{MODE_PSK125R,nullptr,"PSK125R","PSK-125R","PSK125R","PSK125R","PSK","","P125R",  0 | 0  },
{MODE_PSK250R,nullptr,"PSK250R","PSK-250R","PSK250R","PSK250R","PSK","","P250R",  0 | 0  },
{MODE_PSK500R,nullptr,"PSK500R","PSK-500R","PSK500R","PSK500R","PSK","","P500R",  0 | 0  },
{MODE_PSK1000R,nullptr,"PSK1000R","PSK-1000R","PSK1000R","PSK1000R","PSK","","PSK1000R",  0 | 0  },

{MODE_4X_PSK63R,nullptr,"PSK63RC4","4xPSK63R","PSK63RC4","PSK63RC4","PSK","","P63R4", 0 | 0 },
{MODE_5X_PSK63R,nullptr,"PSK63RC5","5xPSK63R","PSK63RC5","PSK63RC5","PSK","","P63R5", 0 | 0 },
{MODE_10X_PSK63R,nullptr,"PSK63RC10","10xPSK63R","PSK63RC10","PSK63RC10","PSK","","P63R10", 0 | 0 },
{MODE_20X_PSK63R,nullptr,"PSK63RC20","20xPSK63R","PSK63RC20","PSK63RC20","PSK","","P63R20", 0 | 0 },
{MODE_32X_PSK63R,nullptr,"PSK63RC32","32xPSK63R","PSK63RC32","PSK63RC32","PSK","","P63R32", 0 | 0 },

{MODE_4X_PSK125R,nullptr,"PSK125RC4","4xPSK125R","PSK125RC4","PSK125RC4","PSK","","P125R4", 0 | 0 },
{MODE_5X_PSK125R,nullptr,"PSK125RC5","5xPSK125R","PSK125RC5","PSK125RC5","PSK","","P125R5", 0 | 0 },
{MODE_10X_PSK125R,nullptr,"PSK125RC10","10xPSK125R","PSK125RC10","PSK125RC10","PSK","","P125R10", 0 | 0 },
{MODE_12X_PSK125R,nullptr,"PSK125RC12","12xPSK125R","PSK125RC12","PSK125RC12","PSK","","P125R12", 0 | 0 },
{MODE_16X_PSK125R,nullptr,"PSK125RC16","16xPSK125R","PSK125RC16","PSK125RC16","PSK","","P125R16", 0 | 0 },

{MODE_2X_PSK250R,nullptr,"PSK250RC2","2xPSK250R","PSK250RC2","PSK250RC2","PSK","","P250R2", 0 | 0 },
{MODE_3X_PSK250R,nullptr,"PSK250RC3","3xPSK250R","PSK250RC3","PSK250RC3","PSK","","P250R3", 0 | 0 },
{MODE_5X_PSK250R,nullptr,"PSK250RC5","5xPSK250R","PSK250RC5","PSK250RC5","PSK","","P250R5", 0 | 0 },
{MODE_6X_PSK250R,nullptr,"PSK250RC6","6xPSK250R","PSK250RC6","PSK250RC6","PSK","","P250R6", 0 | 0 },
{MODE_7X_PSK250R,nullptr,"PSK250RC7","7xPSK250R","PSK250RC7","PSK250RC7","PSK","","P250R7", 0 | 0 },

{MODE_2X_PSK500R,nullptr,"PSK500RC2","2xPSK500R","PSK500RC2","PSK500RC2","PSK","","P500R2", 0 | 0 },
{MODE_3X_PSK500R,nullptr,"PSK500RC3","3xPSK500R","PSK500RC3","PSK500RC3","PSK","","P500R3", 0 | 0 },
{MODE_4X_PSK500R,nullptr,"PSK500RC4","4xPSK500R","PSK500RC4", "PSK500RC4","PSK","","P500R4", 0 | 0 },

{MODE_2X_PSK800R,nullptr,"PSK800RC2","2xPSK800R","PSK800RC2","PSK800RC2","PSK","","P800RC2", 0 | 0 },

{MODE_2X_PSK1000R,nullptr,"PSK1000RC2","2xPSK1000R","PSK1000RC2","PSK1000RC2","PSK","","P1KRC2", 0 | 0 },

{MODE_FSQ,nullptr,"FSQ","FSQ","FSQ","FSQ","FSQ","","FSQ", 0 },
{MODE_IFKP,nullptr,"IFKP","IFKP","IFKP","IFKP","IFKP","","IFKP", 0 },

{MODE_SSB,nullptr,"SSB","SSB","","SSB","SSB","","", 0 },
{MODE_WWV,nullptr,"WWV","WWV","","","","","", 0 },
{MODE_ANALYSIS,nullptr,"ANALYSIS","Freq Analysis","","","","","", 0 },
{MODE_FMT,nullptr,"FMT","Frequency Measurement Test","","","","","", 0 },

{MODE_EOT,nullptr,"RSID_EOT","EOT","","EOT","","","", 0 }
};
const char* web_family(int mode) { switch(mode) {
case MODE_CW: return "CW";
case MODE_CONTESTIA: return "Contestia";
case MODE_CONTESTIA_4_125: return "Contestia";
case MODE_CONTESTIA_4_250: return "Contestia";
case MODE_CONTESTIA_4_500: return "Contestia";
case MODE_CONTESTIA_4_1000: return "Contestia";
case MODE_CONTESTIA_4_2000: return "Contestia";
case MODE_CONTESTIA_8_125: return "Contestia";
case MODE_CONTESTIA_8_250: return "Contestia";
case MODE_CONTESTIA_8_500: return "Contestia";
case MODE_CONTESTIA_8_1000: return "Contestia";
case MODE_CONTESTIA_8_2000: return "Contestia";
case MODE_CONTESTIA_16_250: return "Contestia";
case MODE_CONTESTIA_16_500: return "Contestia";
case MODE_CONTESTIA_16_1000: return "Contestia";
case MODE_CONTESTIA_16_2000: return "Contestia";
case MODE_CONTESTIA_32_1000: return "Contestia";
case MODE_CONTESTIA_32_2000: return "Contestia";
case MODE_CONTESTIA_64_500: return "Contestia";
case MODE_CONTESTIA_64_1000: return "Contestia";
case MODE_CONTESTIA_64_2000: return "Contestia";
case MODE_DOMINOEXMICRO: return "DominoEX";
case MODE_DOMINOEX4: return "DominoEX";
case MODE_DOMINOEX5: return "DominoEX";
case MODE_DOMINOEX8: return "DominoEX";
case MODE_DOMINOEX11: return "DominoEX";
case MODE_DOMINOEX16: return "DominoEX";
case MODE_DOMINOEX22: return "DominoEX";
case MODE_DOMINOEX44: return "DominoEX";
case MODE_DOMINOEX88: return "DominoEX";
case MODE_FELDHELL: return "Hellschreiber";
case MODE_SLOWHELL: return "Hellschreiber";
case MODE_HELLX5: return "Hellschreiber";
case MODE_HELLX9: return "Hellschreiber";
case MODE_FSKH245: return "Hellschreiber";
case MODE_FSKH105: return "Hellschreiber";
case MODE_HELL80: return "Hellschreiber";
case MODE_MFSK8: return "MFSK";
case MODE_MFSK16: return "MFSK";
case MODE_MFSK32: return "MFSK";
case MODE_MFSK4: return "MFSK";
case MODE_MFSK11: return "MFSK";
case MODE_MFSK22: return "MFSK";
case MODE_MFSK31: return "MFSK";
case MODE_MFSK64: return "MFSK";
case MODE_MFSK128: return "MFSK";
case MODE_MFSK64L: return "MFSK";
case MODE_MFSK128L: return "MFSK";
case MODE_WEFAX_576: return "WEFAX";
case MODE_WEFAX_288: return "WEFAX";
case MODE_NAVTEX: return "NAVTEX";
case MODE_SITORB: return "NAVTEX";
case MODE_MT63_500S: return "MT63";
case MODE_MT63_500L: return "MT63";
case MODE_MT63_1000S: return "MT63";
case MODE_MT63_1000L: return "MT63";
case MODE_MT63_2000S: return "MT63";
case MODE_MT63_2000L: return "MT63";
case MODE_PSK31: return "PSK";
case MODE_PSK63: return "PSK";
case MODE_PSK63F: return "PSK";
case MODE_PSK125: return "PSK";
case MODE_PSK250: return "PSK";
case MODE_PSK500: return "PSK";
case MODE_PSK1000: return "PSK";
case MODE_12X_PSK125: return "PSK";
case MODE_6X_PSK250: return "PSK";
case MODE_2X_PSK500: return "PSK";
case MODE_4X_PSK500: return "PSK";
case MODE_2X_PSK800: return "PSK";
case MODE_2X_PSK1000: return "PSK";
case MODE_QPSK31: return "PSK";
case MODE_QPSK63: return "PSK";
case MODE_QPSK125: return "PSK";
case MODE_QPSK250: return "PSK";
case MODE_QPSK500: return "PSK";
case MODE_8PSK125: return "PSK";
case MODE_8PSK125FL: return "PSK";
case MODE_8PSK125F: return "PSK";
case MODE_8PSK250: return "PSK";
case MODE_8PSK250FL: return "PSK";
case MODE_8PSK250F: return "PSK";
case MODE_8PSK500: return "PSK";
case MODE_8PSK500F: return "PSK";
case MODE_8PSK1000: return "PSK";
case MODE_8PSK1000F: return "PSK";
case MODE_8PSK1200F: return "PSK";
case MODE_OFDM_500F: return "PSK";
case MODE_OFDM_750F: return "PSK";
case MODE_OFDM_3500: return "PSK";
case MODE_OLIVIA: return "Olivia";
case MODE_OLIVIA_4_125: return "Olivia";
case MODE_OLIVIA_4_250: return "Olivia";
case MODE_OLIVIA_4_500: return "Olivia";
case MODE_OLIVIA_4_1000: return "Olivia";
case MODE_OLIVIA_4_2000: return "Olivia";
case MODE_OLIVIA_8_125: return "Olivia";
case MODE_OLIVIA_8_250: return "Olivia";
case MODE_OLIVIA_8_500: return "Olivia";
case MODE_OLIVIA_8_1000: return "Olivia";
case MODE_OLIVIA_8_2000: return "Olivia";
case MODE_OLIVIA_16_500: return "Olivia";
case MODE_OLIVIA_16_1000: return "Olivia";
case MODE_OLIVIA_16_2000: return "Olivia";
case MODE_OLIVIA_32_1000: return "Olivia";
case MODE_OLIVIA_32_2000: return "Olivia";
case MODE_OLIVIA_64_500: return "Olivia";
case MODE_OLIVIA_64_1000: return "Olivia";
case MODE_OLIVIA_64_2000: return "Olivia";
case MODE_RTTY: return "RTTY";
case MODE_SCAMPFSK: return "SCAMP";
case MODE_SCAMPOOK: return "SCAMP";
case MODE_SCFSKFST: return "SCAMP";
case MODE_SCFSKSLW: return "SCAMP";
case MODE_SCOOKSLW: return "SCAMP";
case MODE_SCFSKVSL: return "SCAMP";
case MODE_THORMICRO: return "THOR";
case MODE_THOR4: return "THOR";
case MODE_THOR5: return "THOR";
case MODE_THOR8: return "THOR";
case MODE_THOR11: return "THOR";
case MODE_THOR16: return "THOR";
case MODE_THOR22: return "THOR";
case MODE_THOR25: return "THOR";
case MODE_THOR32: return "THOR";
case MODE_THOR44: return "THOR";
case MODE_THOR56: return "THOR";
case MODE_THOR25x4: return "THOR";
case MODE_THOR50x1: return "THOR";
case MODE_THOR50x2: return "THOR";
case MODE_THOR100: return "THOR";
case MODE_THROB1: return "Throb";
case MODE_THROB2: return "Throb";
case MODE_THROB4: return "Throb";
case MODE_THROBX1: return "Throb";
case MODE_THROBX2: return "Throb";
case MODE_THROBX4: return "Throb";
case MODE_PSK125R: return "PSK";
case MODE_PSK250R: return "PSK";
case MODE_PSK500R: return "PSK";
case MODE_PSK1000R: return "PSK";
case MODE_4X_PSK63R: return "PSK";
case MODE_5X_PSK63R: return "PSK";
case MODE_10X_PSK63R: return "PSK";
case MODE_20X_PSK63R: return "PSK";
case MODE_32X_PSK63R: return "PSK";
case MODE_4X_PSK125R: return "PSK";
case MODE_5X_PSK125R: return "PSK";
case MODE_10X_PSK125R: return "PSK";
case MODE_12X_PSK125R: return "PSK";
case MODE_16X_PSK125R: return "PSK";
case MODE_2X_PSK250R: return "PSK";
case MODE_3X_PSK250R: return "PSK";
case MODE_5X_PSK250R: return "PSK";
case MODE_6X_PSK250R: return "PSK";
case MODE_7X_PSK250R: return "PSK";
case MODE_2X_PSK500R: return "PSK";
case MODE_3X_PSK500R: return "PSK";
case MODE_4X_PSK500R: return "PSK";
case MODE_2X_PSK800R: return "PSK";
case MODE_2X_PSK1000R: return "PSK";
case MODE_FSQ: return "FSQ";
case MODE_IFKP: return "IFKP";
case WEB_MODE_DTMF: return "DTMF";
default: return ""; }}