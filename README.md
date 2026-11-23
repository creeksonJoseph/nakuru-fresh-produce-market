# Nakuru Fresh Produce Market

A small C++17 + HTML/CSS/vanilla JavaScript prototype for managing produce inventory, retailer orders, dispatches, alerts and daily sales.

## Requirements

- Ubuntu/Linux
- g++ with C++17 support
- SQLite development library
- PostgreSQL client development library (only needed when building with PostgreSQL support)

Install dependencies:

```bash
sudo apt update
sudo apt install -y g++ libsqlite3-dev libpq-dev
```

## Run locally (SQLite)

```bash
g++ -std=c++17 -O2 -Wall -Wextra main.cpp -lsqlite3 -lpq -o market_app
./market_app
```

Open http://localhost:8080. By default, data is stored in `data/market.db`.

You can override the SQLite file path:

```bash
DATABASE_PATH=/absolute/path/market.db ./market_app
```

## Use PostgreSQL locally or online

When `DATABASE_URL` is set, the application uses PostgreSQL instead of SQLite. When it is absent, the app uses SQLite.

```bash
export DATABASE_URL='postgresql://USER:PASSWORD@HOST:5432/DATABASE?sslmode=require'
./market_app
```

Use the connection string supplied by your database provider. Do not commit it to GitHub or place it in the Dockerfile.

The app creates its three tables automatically on first connection: `batches`, `orders`, and `deliveries`.

## Run with Docker

Build:

```bash
docker build -t nakuru-market .
```

Run locally with a persistent SQLite volume:

```bash
docker run --rm -p 8080:10000 \
  -e PORT=10000 \
  -e DATABASE_PATH=/app/data/market.db \
  -v nakuru-market-data:/app/data \
  nakuru-market
```

Or run the container against PostgreSQL:

```bash
docker run --rm -p 8080:10000 \
  -e PORT=10000 \
  -e DATABASE_URL='postgresql://USER:PASSWORD@HOST:5432/DATABASE?sslmode=require' \
  nakuru-market
```

## Deploy to Render

This repository includes a `render.yaml` Blueprint and a Dockerfile configured for Render's Free web service.

1. In Render, open the PostgreSQL database you created.
2. Copy its **Internal Database URL** if the web service is in the same Render region. If it is in a different region, use the External Database URL.
3. Open the Nakuru web service → **Environment**.
4. Add an environment variable:
   - **Key:** `DATABASE_URL`
   - **Value:** paste the PostgreSQL connection URL from Render.
5. Save changes and trigger a deploy if Render does not automatically redeploy.

Do not add the connection URL to the Dockerfile, `render.yaml`, source code, or GitHub. Render injects environment variables into the running service.

The app checks for `DATABASE_URL` at startup. If it is set, it connects to PostgreSQL; otherwise it falls back to local SQLite. The Render database URL should include SSL settings if Render provides them. Render's own connection URL is the source of truth.

The web service listens on Render's assigned `PORT` and exposes `/healthz` as its health check.

## Features

- Dashboard KPIs for orders, revenue, low stock and expiring batches
- Inventory and batch intake
- Retailer orders with FIFO stock allocation by earliest expiry date
- Driver dispatch and delivery manifest
- Spoilage / low-stock alerts
- Daily sales summary
- PostgreSQL persistence when `DATABASE_URL` is set
- SQLite persistence for local development by default
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

This is an academic demonstration, not a production-ready commerce system. Authentication and authorization, a production-grade HTTP framework, robust JSON parsing, database migrations, automated backups and multi-instance concurrency controls are not implemented.
