FROM node:22-bookworm-slim

WORKDIR /app

RUN apt-get update \
  && apt-get install -y --no-install-recommends openssl python3 make g++ \
  && rm -rf /var/lib/apt/lists/*

COPY package*.json ./
RUN npm install

COPY . .

RUN mkdir -p /app/data \
  && npm run db:generate

EXPOSE 3000

CMD ["sh", "-c", "npm run db:deploy && npm run dev -- --hostname 0.0.0.0"]
