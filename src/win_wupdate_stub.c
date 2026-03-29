/**
 * MinGW / 无独立升级库时的占位实现。
 * 返回非 0：与 cFep 菜单逻辑一致（不打印「已是最新版本」）。
 */
#if defined(_WIN32) || defined(__WIN32)

int
wupdate_start(const char *psoftname, const char *psoftver, const char *purlini)
{
    (void)psoftname;
    (void)psoftver;
    (void)purlini;
    return 1;
}

#endif
