CREATE TABLE "delivery" ("id" INTEGER PRIMARY KEY  NOT NULL ,"time" DATETIME,"event" TEXT);
CREATE TABLE "tracking" ("id" INTEGER PRIMARY KEY  NOT NULL ,"tracking_number" TEXT NOT NULL ,"description" TEXT NOT NULL ,"delivery_status" BOOL NOT NULL ,"delivery_date" DATETIME);
