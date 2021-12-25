#define EXPORT __attribute__((visibility("default")))

__attribute__((constructor))
static void initializer(void) {
}

__attribute__((destructor))
static void finalizer(void) {
}

EXPORT
void actor(void) {
}
