# Nakuru Fresh Produce Market

A small C++17 + HTML/CSS/vanilla JavaScript prototype for managing produce inventory, retailer orders, dispatches, alerts and daily sales.

## Requirements

- Ubuntu/Linux
- g++ with C++17 support
- SQLite development library

Install the local dependencies:

```bash
sudo apt update
sudo apt install -y g++ libsqlite3-dev
```

## Run locally

From the project directory:

```bash
g++ -std=c++17 -O2 -Wall -Wextra main.cpp -lsqlite3 -o market_app
./market_app
```

Open http://localhost:8080.

By default, SQLite stores data in `data/market.db`. You can choose another path:

```bash
DATABASE_PATH=/absolute/path/market.db ./market_app
```

The server uses port 8080 locally. Set `PORT` to override it.

## Run with Docker

Build and run with a persistent local volume:

```bash
docker build -t nakuru-market .
docker run --rm -p 8080:10000 \
  -e PORT=10000 \
  -e DATABASE_PATH=/app/data/market.db \
  -v nakuru-market-data:/app/data \
  nakuru-market
```

Open http://localhost:8080.

## Deploy to Render (Free)

This repository includes a `render.yaml` Blueprint and a multi-stage `Dockerfile`.

1. Push the repository to GitHub.
2. In Render, choose **New → Blueprint**.
3. Connect this repository and deploy the Blueprint.
4. Render creates the web service using the Free plan and Dockerfile.

The service listens on Render's assigned `PORT` and exposes `/healthz` as its health check.

### Important: Free-plan data persistence

The Render Free web service has an ephemeral filesystem and no persistent disk. This configuration stores SQLite at `/tmp/nakuru-market.db`, so data survives normal requests and process activity while the instance remains alive, but **can be lost when Render spins the service down, restarts it, or redeploys it**. It is suitable for a class demo, not for reliable production records.

For persistent online data, the app needs a hosted database such as Neon Postgres and a PostgreSQL persistence adapter. That adapter is not included in this SQLite version. Do not assume that changing `DATABASE_PATH` to a Postgres URL will work.

## Features

- Dashboard KPIs for orders, revenue, low stock and expiring batches
- Inventory and batch intake
- Retailer orders with FIFO stock allocation by earliest expiry date
- Driver dispatch and delivery manifest
- Spoilage / low-stock alerts
- Daily sales summary
- SQLite persistence for batches, orders and deliveries
- `GET /healthz` health-check endpoint

## API routes

| Method | Route | Purpose |
|---|---|---|
| GET | `/api/dashboard` | Dashboard KPIs |
| GET | `/api/inventory` | Inventory batches |
| GET | `/api/products` | Produce catalogue and stock |
| GET | `/api/retailers` | Retailers |
| GET | `/api/orders` | Orders |
| GET | `/api/drivers` | Drivers |
| GET | `/api/manifest` | Delivery manifest |
| GET | `/api/alerts` | Expiring and low-stock alerts |
| GET | `/api/sales` | Daily sales totals |
| GET | `/healthz` | Render health check |
| POST | `/api/orders` | Create retailer order |
| POST | `/api/batches` | Record batch intake |
| POST | `/api/dispatch` | Dispatch an order |

## OOP concepts demonstrated

- **Encapsulation:** domain data is private and validated through constructors and setters.
- **Abstraction:** `MarketEntity` defines the common `toJson()` and `getSummary()` interface.
- **Inheritance:** `ProduceBatch`, `Retailer`, `Order` and `Delivery` implement the market entity interface; `Retailer` and `Driver` inherit from `Person`.
- **Polymorphism:** `ReportGenerator` works with a vector of `MarketEntity*`.
- **Composition:** `InventoryManager` owns the domain collections and coordinates operations.
- **FIFO:** orders allocate stock from batches with the earliest expiry date first.

## Prototype limitations

This is an academic demonstration, not a production-ready commerce system. Authentication and authorization, a production-grade HTTP framework, robust JSON parsing, database migrations, automated backups and multi-instance concurrency controls are not implemented. Render Free's ephemeral filesystem can lose SQLite data as described above.
