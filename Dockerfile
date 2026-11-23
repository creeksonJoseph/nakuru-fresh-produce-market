# Build stage: compile against both local SQLite and hosted PostgreSQL.
FROM debian:bookworm-slim AS builder
RUN apt-get update && apt-get install -y --no-install-recommends g++ libsqlite3-dev libpq-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY main.cpp index.html ./
RUN g++ -std=c++17 -O2 -Wall -Wextra main.cpp -lsqlite3 -lpq -o market_app

# Runtime stage contains only the required database client libraries.
FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends libsqlite3-0 libpq5 ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=builder /app/market_app ./market_app
COPY index.html ./index.html
ENV PORT=10000
ENV DATABASE_PATH=/tmp/nakuru-market.db
EXPOSE 10000
CMD ["./market_app"]
