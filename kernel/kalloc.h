struct mmap_area {
    uint64 addr;
    int length;
    int prot;
    int flags;
    struct file *f;
    int offset;
    struct proc *p;
};