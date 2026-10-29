-- 로컬 개발 전용 계약 v3. 기존 운영 DB에는 적용하지 않는다.
-- 기존 wire 길이는 유지하되 DB에는 PBKDF2-HMAC-SHA256 salt/hash만 저장한다.
SET XACT_ABORT ON;
IF OBJECT_ID('dbo.WodLocalSchema','U') IS NULL
BEGIN
    IF OBJECT_ID('dbo.user_data','U') IS NOT NULL THROW 50000,'Unmanaged existing schema: refusing bootstrap',1;
    CREATE TABLE dbo.WodLocalSchema(Version int NOT NULL PRIMARY KEY);
    INSERT dbo.WodLocalSchema VALUES(1);
END;
IF NOT EXISTS(SELECT 1 FROM dbo.WodLocalSchema WHERE Version IN (1,2,3)) THROW 50000,'Unsupported local schema',1;
IF OBJECT_ID('dbo.user_data','U') IS NULL
CREATE TABLE dbo.user_data(
    Player_ID varchar(10) NOT NULL PRIMARY KEY,
    Player_Password varchar(256) NOT NULL,
    Token int NOT NULL DEFAULT 2000 CHECK(Token>=0),
    FullNode bit NOT NULL DEFAULT 0,
    StakedToken int NOT NULL DEFAULT 0,
    StakedDays int NOT NULL DEFAULT 0,
    StakedAt datetime2 NOT NULL DEFAULT SYSDATETIME(),
    Chr_Sex smallint NOT NULL DEFAULT 0,
    Chr_HeadCoverings_Base_Hair smallint NOT NULL DEFAULT 1,
    Chr_HeadCoverings_No_FacialHair smallint NOT NULL DEFAULT 0,
    Chr_HeadCoverings_No_Hair smallint NOT NULL DEFAULT 0,
    Chr_Hair smallint NOT NULL DEFAULT 1,
    Chr_HelmetAttachment smallint NOT NULL DEFAULT 1,
    Chr_BackAttachment smallint NOT NULL DEFAULT 3,
    Chr_ShoulderAttachRight smallint NOT NULL DEFAULT 1,
    Chr_ShoulderAttachLeft smallint NOT NULL DEFAULT 0,
    Chr_ElbowAttachRight smallint NOT NULL DEFAULT 1,
    Chr_ElbowAttachLeft smallint NOT NULL DEFAULT 0,
    Chr_HipsAttachment smallint NOT NULL DEFAULT 0,
    Chr_KneeAttachRight smallint NOT NULL DEFAULT 1,
    Chr_KneeAttachLeft smallint NOT NULL DEFAULT 1,
    Chr_Ear_Ear smallint NOT NULL DEFAULT 1,
    Chr_Head smallint NOT NULL DEFAULT 1,
    Chr_Head_No_Elements smallint NOT NULL DEFAULT 0,
    Chr_Eyebrow smallint NOT NULL DEFAULT 1,
    Chr_Torso smallint NOT NULL DEFAULT 3,
    Chr_ArmUpperRight smallint NOT NULL DEFAULT 2,
    Chr_ArmUpperLeft smallint NOT NULL DEFAULT 2,
    Chr_ArmLowerRight smallint NOT NULL DEFAULT 2,
    Chr_ArmLowerLeft smallint NOT NULL DEFAULT 2,
    Chr_HandRight smallint NOT NULL DEFAULT 1,
    Chr_HandLeft smallint NOT NULL DEFAULT 1,
    Chr_Hips smallint NOT NULL DEFAULT 1,
    Chr_LegRight smallint NOT NULL DEFAULT 1,
    Chr_LegLeft smallint NOT NULL DEFAULT 1
);
IF OBJECT_ID('dbo.customize_data','U') IS NULL
CREATE TABLE dbo.customize_data(Player_ID varchar(10) NOT NULL REFERENCES dbo.user_data(Player_ID),ShopType smallint NOT NULL,ShopNum smallint NOT NULL,PartCount smallint NOT NULL CHECK(PartCount>=0),PRIMARY KEY(Player_ID,ShopType,ShopNum));
IF OBJECT_ID('dbo.auction_data','U') IS NULL
CREATE TABLE dbo.auction_data(Player_ID varchar(10) NOT NULL REFERENCES dbo.user_data(Player_ID),ShopType smallint NOT NULL,ShopNum smallint NOT NULL,BuyPrice int NOT NULL CHECK(BuyPrice BETWEEN 1 AND 65535),Deadline datetime2 NOT NULL,PRIMARY KEY(Player_ID,ShopType,ShopNum));
ALTER TABLE dbo.user_data ALTER COLUMN Player_Password varchar(256) NOT NULL;
IF COL_LENGTH('dbo.user_data','BalanceVersion') IS NULL
ALTER TABLE dbo.user_data ADD BalanceVersion rowversion;
IF OBJECT_ID('dbo.reward_applied','U') IS NULL
CREATE TABLE dbo.reward_applied(Operation_ID varchar(32) NOT NULL PRIMARY KEY, Player_ID varchar(10) NOT NULL REFERENCES dbo.user_data(Player_ID),TokenDelta int NOT NULL,AppliedAt datetime2 NOT NULL DEFAULT SYSDATETIME());
GO
CREATE OR ALTER PROCEDURE dbo.SignUp @id varchar(10), @password varchar(256) AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
IF DATALENGTH(@password)<>118 OR LEFT(@password,21) COLLATE Latin1_General_100_BIN2 <> 'pbkdf2-sha256$600000$' OR SUBSTRING(@password,54,1)<>'$' OR SUBSTRING(@password,22,32) COLLATE Latin1_General_100_BIN2 LIKE '%[^0-9a-f]%' OR SUBSTRING(@password,55,64) COLLATE Latin1_General_100_BIN2 LIKE '%[^0-9a-f]%' THROW 50022,'Hashed credential required',1;
INSERT dbo.user_data(Player_ID,Player_Password) VALUES(@id,@password);
END;
GO
CREATE OR ALTER PROCEDURE dbo.GetPlayerInfo @id varchar(10), @password varchar(256) AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
SELECT Token,Chr_Sex,Chr_HeadCoverings_Base_Hair,Chr_HeadCoverings_No_FacialHair,Chr_HeadCoverings_No_Hair,Chr_Hair,Chr_HelmetAttachment,Chr_BackAttachment,Chr_ShoulderAttachRight,Chr_ShoulderAttachLeft,Chr_ElbowAttachRight,Chr_ElbowAttachLeft,Chr_HipsAttachment,Chr_KneeAttachRight,Chr_KneeAttachLeft,Chr_Ear_Ear,Chr_Head,Chr_Head_No_Elements,Chr_Eyebrow,Chr_Torso,Chr_ArmUpperRight,Chr_ArmUpperLeft,Chr_ArmLowerRight,Chr_ArmLowerLeft,Chr_HandRight,Chr_HandLeft,Chr_Hips,Chr_LegRight,Chr_LegLeft,CONVERT(bigint,BalanceVersion) FROM dbo.user_data WHERE Player_ID=@id AND Player_Password=@password;
END;
GO
CREATE OR ALTER PROCEDURE dbo.SavePlayerInfo @id varchar(10), @token int, @p0 smallint, @p1 smallint, @p2 smallint, @p3 smallint, @p4 smallint, @p5 smallint, @p6 smallint, @p7 smallint, @p8 smallint, @p9 smallint, @p10 smallint, @p11 smallint, @p12 smallint, @p13 smallint, @p14 smallint, @p15 smallint, @p16 smallint, @p17 smallint, @p18 smallint, @p19 smallint, @p20 smallint, @p21 smallint, @p22 smallint, @p23 smallint, @p24 smallint, @p25 smallint, @p26 smallint, @p27 smallint AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
UPDATE dbo.user_data SET Token=@token,Chr_Sex=@p0,Chr_HeadCoverings_Base_Hair=@p1,Chr_HeadCoverings_No_FacialHair=@p2,Chr_HeadCoverings_No_Hair=@p3,Chr_Hair=@p4,Chr_HelmetAttachment=@p5,Chr_BackAttachment=@p6,Chr_ShoulderAttachRight=@p7,Chr_ShoulderAttachLeft=@p8,Chr_ElbowAttachRight=@p9,Chr_ElbowAttachLeft=@p10,Chr_HipsAttachment=@p11,Chr_KneeAttachRight=@p12,Chr_KneeAttachLeft=@p13,Chr_Ear_Ear=@p14,Chr_Head=@p15,Chr_Head_No_Elements=@p16,Chr_Eyebrow=@p17,Chr_Torso=@p18,Chr_ArmUpperRight=@p19,Chr_ArmUpperLeft=@p20,Chr_ArmLowerRight=@p21,Chr_ArmLowerLeft=@p22,Chr_HandRight=@p23,Chr_HandLeft=@p24,Chr_Hips=@p25,Chr_LegRight=@p26,Chr_LegLeft=@p27 WHERE Player_ID=@id; IF @@ROWCOUNT<>1 THROW 50010,'Unknown account',1;
END;
GO
CREATE OR ALTER PROCEDURE dbo.SaveToken @id varchar(10), @delta int AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
UPDATE dbo.user_data SET Token=Token+@delta WHERE Player_ID=@id AND CONVERT(bigint,Token)+@delta BETWEEN 0 AND 2147483647;
IF @@ROWCOUNT<>1 THROW 50011,'Insufficient tokens or unknown account',1;
END;
GO
CREATE OR ALTER PROCEDURE dbo.SaveShopBuyInfo @id varchar(10), @type int, @num int AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
UPDATE dbo.customize_data WITH (UPDLOCK,SERIALIZABLE) SET PartCount=PartCount+1 WHERE Player_ID=@id AND ShopType=@type AND ShopNum=@num;
IF @@ROWCOUNT=0 INSERT dbo.customize_data VALUES(@id,@type,@num,1);
END;
GO
CREATE OR ALTER PROCEDURE dbo.RegisterAuction @id varchar(10), @type int, @num int, @price int, @deadline datetime2 AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
BEGIN TRY
BEGIN TRAN;
IF @price<=0 OR @price>65535 THROW 50012,'Invalid price',1;
UPDATE dbo.customize_data SET PartCount=PartCount-1 WHERE Player_ID=@id AND ShopType=@type AND ShopNum=@num AND PartCount>0;
IF @@ROWCOUNT<>1 THROW 50013,'Part not owned',1;
IF EXISTS(SELECT 1 FROM dbo.auction_data WITH (UPDLOCK,HOLDLOCK) WHERE Player_ID=@id AND ShopType=@type AND ShopNum=@num) THROW 50014,'Part already listed',1;
INSERT dbo.auction_data VALUES(@id,@type,@num,@price,@deadline);
COMMIT;
END TRY BEGIN CATCH IF @@TRANCOUNT>0 ROLLBACK; THROW; END CATCH;
END;
GO
CREATE OR ALTER PROCEDURE dbo.BuyAuction @seller varchar(10), @buyer varchar(10), @type smallint, @num smallint, @price int AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
IF @@TRANCOUNT=0 THROW 50015,'Purchase requires transaction',1;
IF @seller=@buyer THROW 50016,'Self purchase',1;
DELETE dbo.auction_data WHERE Player_ID=@seller AND ShopType=@type AND ShopNum=@num AND BuyPrice=@price AND Deadline>SYSDATETIME();
IF @@ROWCOUNT<>1 THROW 50017,'Listing unavailable',1;
EXEC dbo.SaveShopBuyInfo @buyer,@type,@num;
END;
GO
CREATE OR ALTER PROCEDURE dbo.SettleExpiredAuctions AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
BEGIN TRY
BEGIN TRAN;
DECLARE @expired TABLE(Player_ID varchar(10),ShopType smallint,ShopNum smallint);
DELETE dbo.auction_data OUTPUT deleted.Player_ID,deleted.ShopType,deleted.ShopNum INTO @expired WHERE Deadline<=SYSDATETIME();
UPDATE owned SET PartCount=owned.PartCount+1 FROM dbo.customize_data owned JOIN @expired expired ON owned.Player_ID=expired.Player_ID AND owned.ShopType=expired.ShopType AND owned.ShopNum=expired.ShopNum;
INSERT dbo.customize_data SELECT expired.Player_ID,expired.ShopType,expired.ShopNum,1 FROM @expired expired WHERE NOT EXISTS(SELECT 1 FROM dbo.customize_data WITH(UPDLOCK,HOLDLOCK) WHERE Player_ID=expired.Player_ID AND ShopType=expired.ShopType AND ShopNum=expired.ShopNum);
COMMIT;
END TRY BEGIN CATCH IF @@TRANCOUNT>0 ROLLBACK; THROW; END CATCH;
END;
GO
CREATE OR ALTER PROCEDURE dbo.GetAuctionInfo @page int AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
IF @page<0 THROW 50018,'Invalid page',1;
EXEC dbo.SettleExpiredAuctions;
SELECT Player_ID,ShopType,ShopNum,BuyPrice,Deadline FROM dbo.auction_data ORDER BY Deadline,Player_ID,ShopType,ShopNum OFFSET (CONVERT(bigint,@page)*7) ROWS FETCH NEXT 7 ROWS ONLY;
END;
GO
CREATE OR ALTER PROCEDURE dbo.GetAuctionPartsNum AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
EXEC dbo.SettleExpiredAuctions;
SELECT COUNT(*) FROM dbo.auction_data;
END;
GO
CREATE OR ALTER PROCEDURE dbo.GetPlayerCustomizeParts @id varchar(10) AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
SELECT ShopType,ShopNum,PartCount FROM dbo.customize_data WHERE Player_ID=@id AND PartCount>0 ORDER BY ShopType,ShopNum;
END;
GO
CREATE OR ALTER PROCEDURE dbo.GetPlayersByShopType @type smallint AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
SELECT DISTINCT Player_ID FROM dbo.customize_data WHERE ShopType=@type AND PartCount>0;
END;
GO
CREATE OR ALTER PROCEDURE dbo.StakeToken @id varchar(10), @token int, @days int AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
UPDATE dbo.user_data SET Token=Token-@token, StakedToken=StakedToken+@token, StakedDays=@days, StakedAt=SYSDATETIME() WHERE Player_ID=@id AND Token>=@token AND @token>=10 AND @days>=100;
IF @@ROWCOUNT<>1 THROW 50019,'Stake rejected',1;
END;
GO
CREATE OR ALTER PROCEDURE dbo.ChangeNode @id varchar(10), @full bit AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
UPDATE dbo.user_data SET FullNode=@full WHERE Player_ID=@id; IF @@ROWCOUNT<>1 THROW 50020,'Unknown account',1;
END;
GO
CREATE OR ALTER PROCEDURE dbo.GetFullNodeInfo @id varchar(10) AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
SELECT FullNode FROM dbo.user_data WHERE Player_ID=@id;
END;
GO
CREATE OR ALTER PROCEDURE dbo.GetStakedTokenData @id varchar(10) AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
SELECT StakedToken,StakedDays,DATEDIFF(day,StakedAt,SYSDATETIME()) FROM dbo.user_data WHERE Player_ID=@id;
END;
GO
CREATE OR ALTER PROCEDURE dbo.GetValidatorIDs AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
SELECT Player_ID,StakedToken FROM dbo.user_data WHERE FullNode=1 AND StakedToken>0;
END;
GO
CREATE OR ALTER PROCEDURE dbo.ApplyReward @operation varchar(32),@id varchar(10),@delta int AS
BEGIN
SET NOCOUNT ON;
SET XACT_ABORT ON;
BEGIN TRY
BEGIN TRAN;
IF EXISTS(SELECT 1 FROM dbo.reward_applied WITH(UPDLOCK,HOLDLOCK) WHERE Operation_ID=@operation)
BEGIN
    IF NOT EXISTS(SELECT 1 FROM dbo.reward_applied WHERE Operation_ID=@operation AND Player_ID=@id AND TokenDelta=@delta) THROW 50021,'Conflicting reward operation',1;
END
ELSE
BEGIN
    EXEC dbo.SaveToken @id,@delta;
    INSERT dbo.reward_applied(Operation_ID,Player_ID,TokenDelta) VALUES(@operation,@id,@delta);
END;
COMMIT;
END TRY BEGIN CATCH IF @@TRANCOUNT>0 ROLLBACK; THROW; END CATCH;
END;
GO
-- Version 3 is committed by Initialize-LocalDatabase.ps1 after credential migration.
