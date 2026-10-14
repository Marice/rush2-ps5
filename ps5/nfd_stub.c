/* nativefiledialog-extended for the PS5: a title has no file dialogs, so every
 * dialog reports a cancel. The ROM is found next to the title instead. */
#include <nfd.h>

nfdresult_t NFD_Init(void) { return NFD_OKAY; }
void NFD_Quit(void) {}
const char *NFD_GetError(void) { return "no file dialogs on the PS5"; }
void NFD_ClearError(void) {}
void NFD_FreePathN(nfdnchar_t *filePath) { (void)filePath; }

nfdresult_t NFD_OpenDialogN(nfdnchar_t **outPath, const nfdnfilteritem_t *filterList,
                            nfdfiltersize_t filterCount, const nfdnchar_t *defaultPath) {
    (void)outPath; (void)filterList; (void)filterCount; (void)defaultPath;
    return NFD_CANCEL;
}

nfdresult_t NFD_OpenDialogMultipleN(const nfdpathset_t **outPaths, const nfdnfilteritem_t *filterList,
                                    nfdfiltersize_t filterCount, const nfdnchar_t *defaultPath) {
    (void)outPaths; (void)filterList; (void)filterCount; (void)defaultPath;
    return NFD_CANCEL;
}

nfdresult_t NFD_SaveDialogN(nfdnchar_t **outPath, const nfdnfilteritem_t *filterList,
                            nfdfiltersize_t filterCount, const nfdnchar_t *defaultPath,
                            const nfdnchar_t *defaultName) {
    (void)outPath; (void)filterList; (void)filterCount; (void)defaultPath; (void)defaultName;
    return NFD_CANCEL;
}

nfdresult_t NFD_PickFolderN(nfdnchar_t **outPath, const nfdnchar_t *defaultPath) {
    (void)outPath; (void)defaultPath;
    return NFD_CANCEL;
}

nfdresult_t NFD_PathSet_GetCount(const nfdpathset_t *pathSet, nfdpathsetsize_t *count) {
    (void)pathSet;
    *count = 0;
    return NFD_OKAY;
}

nfdresult_t NFD_PathSet_GetPathN(const nfdpathset_t *pathSet, nfdpathsetsize_t index, nfdnchar_t **outPath) {
    (void)pathSet; (void)index; (void)outPath;
    return NFD_ERROR;
}

void NFD_PathSet_FreePathN(const nfdnchar_t *filePath) { (void)filePath; }
void NFD_PathSet_Free(const nfdpathset_t *pathSet) { (void)pathSet; }
