# Nakuru Fresh Produce Market

A small C++17 + HTML/CSS/vanilla JavaScript prototype for managing produce inventory, retailer orders, dispatches, alerts and daily sales. PostgreSQL is the only database used by the application.

## Requirements

- Ubuntu/Linux
- g++ with C++17 support
- PostgreSQL client development library

Install dependencies:

```bash
sudo apt update
sudo apt install -y g++ libpq-dev
```

## Configure local development

The application requires a PostgreSQL connection string in the `DATABASE_URL` environment variable. Use the **External Database URL** from your Render PostgreSQL dashboard when connecting from your local computer.

In the project directory, run:

```bash
export DATABASE_URL='YOUR_RENDER_EXTERNAL_DATABASE_URL'
```

Keep the URL private. Never paste it into source code, commit it to GitHub, or put it in the Dockerfile.

To avoid exporting it every time, create a local `.env` file (already excluded from Git) containing:

```env
DATABASE_URL=YOUR_RENDER_EXTERNAL_DATABASE_URL
```

The C++ application does not automatically read `.env` files. Load it into your shell before running the app:

```bash
set -a
source .env
set +a
```

## Build and run locally

```bash
g++ -std=c++17 -O2 -Wall -Wextra main.cpp -pthread -lpq -o market_app
./market_app
```

Open http://localhost:8080. The application creates its PostgreSQL tables automatically on first startup: `batches`, `orders`, and `deliveries`.

If `DATABASE_URL` is missing or the database connection fails, the application stops with an error. There is no local database fallback.

## Run with Docker

Build:

```bash
docker build -t nakuru-market .
```

Run against your PostgreSQL database:

```bash
docker run --rm -p 8080:10000 \
  -e PORT=10000 \
  -e DATABASE_URL='YOUR_POSTGRES_CONNECTION_URL' \
  nakuru-market
```

## Deploy to Render

This repository includes a `render.yaml` Blueprint and a Dockerfile configured for Render's Free web service.

1. Open your Render PostgreSQL database.
2. Copy its **Internal Database URL** (use this when the web service is in the same Render region).
3. Open the Nakuru web service → **Environment**.
4. Confirm this environment variable exists:
   - **Key:** `DATABASE_URL`
   - **Value:** your Render PostgreSQL Internal Database URL.
5. Save changes and deploy.

Do not add the connection URL to the Dockerfile, `render.yaml`, source code, or GitHub. Render injects environment variables into the running service.

The web service listens on Render's assigned `PORT` and exposes `/healthz` as its health check.

## Features

- Dashboard KPIs for orders, revenue, low stock and expiring batches
- Inventory and batch intake
- Retailer orders with FIFO stock allocation by earliest expiry date
- Driver dispatch and delivery manifest
- Spoilage / low-stock alerts
- Daily sales summary
- PostgreSQL persistence for both local development and production
- Four-worker HTTP request pool so several teammates can use the app at the same time
- Shared inventory and PostgreSQL operations are safely coordinated within the running app process
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

This is an academic demonstration, not a production-ready commerce system. There is no authentication, so anyone with the URL can access the demo. The server uses four request workers, while shared inventory and database operations are serialized with a mutex to protect the in-memory state and the single PostgreSQL connection. This supports several teammates using one running app instance, but it is not designed for multiple app instances or production-scale concurrent writes. A production deployment would also need a production-grade HTTP framework, robust JSON parsing, database migrations and automated backups.
