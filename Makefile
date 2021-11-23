libandroid-shm:
	$(CC) -shared -g -I./include cutils/ashmem-dev.c libandroid-shm.c -Wl,-wrap=mmap -ldl -o libandroid-shm$(LIBSUFFIX).so

shm-launch:
	$(CC) -I./include shm-launch.c -o shm-launch -L./ -landroid-shm$(LIBSUFFIX)

test:
	$(CC) -I./include tests/test-server.c -o test-server -L./ -lrt
	$(CC) -I./include tests/test-client.c -o test-client -L./ -lrt

clean:
	rm *.so shm-launch test-client test-server || true

all: libandroid-shm shm-launch test

install:
	install -D --mode 755 libandroid-shm$(LIBSUFFIX).so $(DESTDIR)./usr/lib/$(TARGET)/libandroid-shm$(LIBSUFFIX).so || true
	install -D --mode 755 shm-launch $(DESTDIR)./usr/bin/shm-launch || true


