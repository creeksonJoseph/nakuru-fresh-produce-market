# Build stage: C++17 application linked only to PostgreSQL's libpq client.
FROM debian:bookworm-slim AS builder
RUN apt-get update && apt-get install -y --no-install-recommends g++ libpq-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY main.cpp index.html ./
RUN g++ -std=c++17 -O2 -Wall -Wextra main.cpp -pthread -lpq -o market_app

# Runtime stage.
FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends libpq5 ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=builder /app/market_app ./market_app
COPY index.html ./index.html
ENV PORT=10000
EXPOSE 10000
CMD ["./market_app"]
