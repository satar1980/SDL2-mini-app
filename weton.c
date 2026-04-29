#include <stdio.h>

// Nama hari
const char *nama_hari[] = {"Sabtu", "Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat"};

// Nama pasaran (siklus 5 hari)
const char *nama_pasaran[] = {"Legi", "Pahing", "Pon", "Wage", "Kliwon"};

// Neptu hari
int neptu_hari[] = {9, 5, 4, 3, 7, 8, 6}; // Sabtu=9, Minggu=5, Senin=4, Selasa=3, Rabu=7, Kamis=8, Jumat=6

// Neptu pasaran
int neptu_pasaran[] = {5, 9, 7, 4, 8}; // Legi=5, Pahing=9, Pon=7, Wage=4, Kliwon=8

// Ramalan berdasarkan neptu total % 5
const char *ramalan[] = {
    "Sri - Banyak rezeki dan keberuntungan",
    "Lungguh - Mendapat kedudukan tinggi",
    "Gedhong - Kaya raya dan banyak harta",
    "Lara - Sering sakit atau kurang sehat",
    "Pati - Sering mendapat cobaan berat"
};

// Fungsi menentukan hari dari tanggal (Rumus Zeller)
int hitung_hari(int tgl, int bln, int thn) {
    if (bln < 3) {
        bln += 12;
        thn--;
    }
    int K = thn % 100;
    int J = thn / 100;
    int h = (tgl + (13 * (bln + 1)) / 5 + K + (K / 4) + (J / 4) + 5 * J) % 7;
    // h: 0=Sabtu, 1=Minggu, 2=Senin, ..., 6=Jumat
    return h;
}

// Fungsi menentukan pasaran berdasarkan tanggal referensi
// Referensi: 1 Januari 1900 adalah hari Senin Wage
int hitung_pasaran(int tgl, int bln, int thn) {
    // Hitung selisih hari dari 1 Januari 1900
    int total_hari = 0;
    int bulan_hari[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // Tahun dari 1900 sampai thn-1
    for (int y = 1900; y < thn; y++) {
        total_hari += (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365;
    }
    
    // Tahun kabisat untuk bulan Februari tahun ini
    int kabisat = (thn % 4 == 0 && (thn % 100 != 0 || thn % 400 == 0));
    
    // Bulan dari Januari sampai bln-1
    for (int m = 1; m < bln; m++) {
        total_hari += bulan_hari[m-1];
        if (m == 2 && kabisat) total_hari++;
    }
    
    // Tanggal
    total_hari += tgl - 1; // karena 1 Jan 1900 adalah hari ke-0
    
    // Pasaran: 1 Jan 1900 = Wage (indeks 3 dari Legi=0)
    // Wage = indeks 3, maka shift = (3 - total_hari) mod 5
    int shift = (3 - (total_hari % 5) + 5) % 5;
    return shift;
}

int main() {
    int tgl, bln, thn;
    int idx_hari, idx_pasaran;
    int neptu_total, ramalan_index;
    
    printf("=== RAMALAN WETON DARI TANGGAL LAHIR ===\n");
    printf("Masukkan tanggal lahir (dd mm yyyy): ");
    scanf("%d %d %d", &tgl, &bln, &thn);
    
    // Validasi sederhana
    if (thn < 1900 || bln < 1 || bln > 12 || tgl < 1 || tgl > 31) {
        printf("Tanggal tidak valid (minimal tahun 1900).\n");
        return 1;
    }
    
    idx_hari = hitung_hari(tgl, bln, thn);
    idx_pasaran = hitung_pasaran(tgl, bln, thn);
    
    neptu_total = neptu_hari[idx_hari] + neptu_pasaran[idx_pasaran];
    ramalan_index = neptu_total % 5;
    
    printf("\n=== HASIL RAMALAN ===\n");
    printf("Tanggal lahir : %02d-%02d-%d\n", tgl, bln, thn);
    printf("Weton        : %s %s\n", nama_hari[idx_hari], nama_pasaran[idx_pasaran]);
    printf("Neptu        : %d + %d = %d\n", neptu_hari[idx_hari], neptu_pasaran[idx_pasaran], neptu_total);
    printf("Ramalan      : %s\n", ramalan[ramalan_index]);
    
    return 0;
}
