FROM emscripten/emsdk:3.1.51 AS wasm-builder

WORKDIR /build/native

COPY native/Makefile .
RUN rm -rf emsdk && ln -s "${EMSDK}" emsdk

RUN make raylib && make ffmpeg && make opencv

COPY native/ .

RUN make clean && make all && make copy DEST=/build/public/wasm

RUN test -s /build/public/wasm/index.data || \
    (echo "ERROR: index.data missing after 'make copy' — check native/Makefile's" \
          "LDFLAGS still has --preload-file assets, and that the assets/ dir" \
          "was actually present when emcc ran." >&2 && exit 1)
RUN test -s /build/public/wasm/index.wasm || (echo "ERROR: index.wasm missing" >&2 && exit 1)
RUN test -s /build/public/wasm/index.js   || (echo "ERROR: index.js missing" >&2 && exit 1)

RUN mkdir -p /build/public/ffmpeg && \
    cp ffmpeg/ffmpeg-core.js ffmpeg/ffmpeg-core.wasm /build/public/ffmpeg/

FROM node:20-alpine AS build

WORKDIR /app
COPY package.json package-lock.json ./
RUN npm ci

COPY . .
COPY --from=wasm-builder /build/public/wasm ./public/wasm
COPY --from=wasm-builder /build/public/ffmpeg ./public/ffmpeg

RUN npm run build

RUN test -s /app/dist/wasm/index.data || \
    (echo "ERROR: index.data didn't make it into dist/wasm — check vite.config.ts's publicDir" >&2 && exit 1)

FROM nginx:1.27-alpine

RUN cat <<'EOF' > /etc/nginx/conf.d/default.conf
server {
    listen 80;
    server_name _;
    root /usr/share/nginx/html;
    index index.html;

    gzip on;
    gzip_types text/plain text/css application/javascript application/wasm application/json;

    location ~* \.wasm$ {
        default_type application/wasm;
        add_header Cross-Origin-Resource-Policy cross-origin;
    }

    location /wasm/ {
        try_files $uri =404;
    }

    location /ffmpeg/ {
        add_header Cross-Origin-Resource-Policy cross-origin;
        try_files $uri =404;
    }

    location / {
        try_files $uri $uri/ /index.html;
    }
}
EOF

COPY --from=build /app/dist /usr/share/nginx/html

EXPOSE 80
