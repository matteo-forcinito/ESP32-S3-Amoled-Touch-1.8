#include <lvgl.h>
#include <SD_MMC.h>

// Driver per LVGL che mappa "S:" sulla SD_MMC

static void *sd_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode) {
    const char *flags = (mode == LV_FS_MODE_WR) ? FILE_WRITE : FILE_READ;
    String fullPath = String("/") + path; // Rimuovi "S:"
    File *f = new File(SD_MMC.open(fullPath, flags));
    if (!*f || !f->available()) {
        delete f;
        return nullptr;
    }
    return f;
}

static lv_fs_res_t sd_close(lv_fs_drv_t *drv, void *file_p) {
    File *f = (File *)file_p;
    if (f) {
        f->close();
        delete f;
    }
    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br) {
    File *f = (File *)file_p;
    *br = f->read((uint8_t *)buf, btr);
    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence) {
    File *f = (File *)file_p;
    if (whence == LV_FS_SEEK_SET)
        f->seek(pos);
    else if (whence == LV_FS_SEEK_CUR)
        f->seek(f->position() + pos);
    else if (whence == LV_FS_SEEK_END)
        f->seek(f->size() - pos);
    return LV_FS_RES_OK;
}

static lv_fs_res_t sd_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p) {
    File *f = (File *)file_p;
    *pos_p = f->position();
    return LV_FS_RES_OK;
}

void lv_fs_sd_init(void) {
    static lv_fs_drv_t drv;
    lv_fs_drv_init(&drv);

    drv.letter = 'S';   // Prefisso: "S:"
    drv.open_cb = sd_open;
    drv.close_cb = sd_close;
    drv.read_cb = sd_read;
    drv.seek_cb = sd_seek;
    drv.tell_cb = sd_tell;
    drv.cache_size = 0;

    lv_fs_drv_register(&drv);

    Serial.println("[LVGL] ✅ Filesystem driver 'S:' registrato per SD_MMC");
}
