# PowerDNS — Province / ISP / Connection-Type Routing 部署指南

本分支在标准 PowerDNS Authoritative Server 基础上扩展了 GeoIP 后端，支持
**MaxMind GeoIP2 Domain / ISP / Country / Connection-Type** 四个辅助数据库，
并新增 `%is` / `%dm` / `%ct` / `%aso` / `%org` 占位符以及 `geoiplookup()`
Lua API 的相应枚举值（`ISP / Domain / ConnectionType / ASO / ORG / ASN2`）。

> 完整功能演示见 README 顶部及上游 docs/backends/geoip.rst。
> 配套前端：[poweradmin (fork)](https://github.com/linwoodpendleton/poweradmin) — 配上即可获得 DNSPod 风格的"线路解析"UI。

---

## 1. 适用平台 & 依赖

实测平台：**Debian 12 (bookworm) x86_64，PHP 8.2**。其它 Ubuntu 20.04+ / Debian 11+ 同理可推。

```bash
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y \
  build-essential autoconf automake bison flex g++ libtool make pkg-config ragel \
  libboost-all-dev libssl-dev libsodium-dev libsqlite3-dev \
  libmaxminddb-dev libmaxminddb0 \
  libyaml-cpp-dev libcurl4-openssl-dev libluajit-5.1-dev luajit \
  default-libmysqlclient-dev mariadb-server mariadb-client \
  libsystemd-dev gawk python3-venv
```

> SQLite 后端可选；如果不用 `--with-modules='... gsqlite3 ...'` 可跳过 sqlite3 包。

---

## 2. 准备 MaxMind 数据库

需要 **5 个 MMDB 文件**放到 `/etc/geoip/`：

| 文件 | 用途 | 必需 |
|------|------|------|
| `GeoIP2-City.mmdb` | 国家 / 省 / 市 / lat-lon | ✅ 必需 |
| `GeoIP2-Country.mmdb` | 国家覆盖（可选，未配置时回落到 City）| 可选 |
| `GeoIP2-ISP.mmdb` | `%is`, `%aso`, `%org` | 可选 |
| `GeoIP2-Domain.mmdb` | `%dm` | 可选 |
| `GeoIP2-Connection-Type.mmdb` | `%ct` | 可选 |

```bash
mkdir -p /etc/geoip
# 把 5 个 mmdb 文件放进去，例如：
mv ~/Downloads/GeoIP2-*.mmdb /etc/geoip/
chmod 644 /etc/geoip/*.mmdb
```

> 没有付费授权的话，可以用 GeoLite2 免费版（精度稍低），但 `GeoIP2-Domain`/`-ISP`/`-Connection-Type` 是付费版独占。

---

## 3. 编译 & 安装

```bash
git clone https://github.com/linwoodpendleton/pdns.git
cd pdns
autoreconf -vfi
./configure --prefix=/usr/local/powerdns \
            --enable-verbose-logging \
            --with-modules='gmysql geoip lua2 bind' \
            --with-lua \
            --with-maxminddb-includedir=/usr/include \
            --with-maxminddb-libdir=/usr/lib/x86_64-linux-gnu
make -j$(nproc)
make install   # manpage 步骤无 sphinx 时会失败但二进制已装好，可忽略
```

确认 features：

```bash
/usr/local/powerdns/sbin/pdns_server --version 2>&1 | head -5
```

应包含 `libmaxminddb` 与 `lua-records`。

---

## 4. MariaDB（gmysql 后端）

如果只用 YAML zone（`launch=geoip`）可跳过本节。配合 poweradmin / 动态 LUA
路由时必须有 MariaDB。

```bash
systemctl start mariadb
mysql <<SQL
CREATE DATABASE IF NOT EXISTS pdns CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER IF NOT EXISTS pdns@localhost IDENTIFIED BY 'pdns';
GRANT ALL ON pdns.* TO pdns@localhost;
FLUSH PRIVILEGES;
SQL
mysql pdns < /path/to/pdns-source/modules/gmysqlbackend/schema.mysql.sql
```

> **避坑**：Debian 默认的 mariadb.service 带 `ProtectSystem=full`，若 my.cnf 的
> `datadir` 指到 `/usr/local/mysql/data`，启动会报 *Read-only file system*。
> 解决：`systemctl edit mariadb` 加 `[Service]\nReadWritePaths=/usr/local/mysql`。

---

## 5. 配置 `/etc/pdns/pdns.conf`

最小可用配置（YAML zone 模式，无 MariaDB）：

```ini
launch=geoip
local-address=0.0.0.0:5300
edns-subnet-processing=yes          # ★ 必开，geoip 才会用 client subnet
enable-lua-records=yes
expand-alias=yes

api=yes
api-key=changeme
webserver=yes
webserver-address=127.0.0.1
webserver-port=8081

# === 本分支扩展的 5 个数据库参数 ===
geoip-database-files=/etc/geoip/GeoIP2-City.mmdb
geoip-database-country-files=/etc/geoip/GeoIP2-Country.mmdb
geoip-database-domain-files=/etc/geoip/GeoIP2-Domain.mmdb
geoip-database-isp-files=/etc/geoip/GeoIP2-ISP.mmdb
geoip-database-connection-files=/etc/geoip/GeoIP2-Connection-Type.mmdb

geoip-zones-file=/etc/pdns/geoip-zones.yaml
loglevel=4
```

如果配合 poweradmin（推荐），并行启用 `gmysql`：

```ini
launch=gmysql,geoip
gmysql-host=127.0.0.1
gmysql-user=pdns
gmysql-password=pdns
gmysql-dbname=pdns
# 其它行同上
```

---

## 6. 占位符示例

`/etc/pdns/geoip-zones.yaml` 演示所有新占位符：

```yaml
domains:
  - domain: geo.example.com
    ttl: 30
    records:
      geo.example.com:
        - soa: ns1.geo.example.com hostmaster.geo.example.com 1 7200 3600 1209600 60
        - ns: ns1.geo.example.com
      ns1.geo.example.com:
        - a: 10.0.0.1
      probe.geo.example.com:
        - txt: 'co=%co cn=%cn re=%re ci=%ci is=%is dm=%dm ct=%ct aso=%aso'
    services:
      'www.geo.example.com':
        - '%co.%re.%is.www'       # cn.gd.china_telecom.www
        - '%co.%re.www'
        - '%co.www'
        - 'default.www'
    mapping_lookup_formats: ['%co.%re.%is', '%co.%re', '%co']
    custom_mapping:
      'cn.gd.china_telecom': '1.1.1.1'
      'cn.gd':               '2.2.2.2'
      'cn':                  '3.3.3.3'
      'default':             '8.8.8.8'
```

| 占位符 | 含义 | 数据库 |
|--------|------|--------|
| `%cn` | continent code (AS/EU/NA/…) | City 或 Country |
| `%co` / `%cc` | country ISO (CN/US) | City 或 Country |
| `%re` | subdivision_1 iso (省/州) | City |
| `%ci` | city name | City |
| `%as` / `%na` | AS 号 / AS 组织名 | ISP 或 City |
| **`%is`** | ISP 名 | **ISP** |
| **`%dm`** | 二级域名 | **Domain** |
| **`%ct`** | 连接类型 | **Connection-Type** |
| **`%aso`** | autonomous_system_organization | ISP / City |
| **`%org`** | organization | ISP |

---

## 7. 启动 & 验证

```bash
mkdir -p /etc/pdns
nohup /usr/local/powerdns/sbin/pdns_server \
      --config-dir=/etc/pdns --daemon=no --guardian=no \
      > /var/log/pdns.log 2>&1 &

# 验证占位符（必须带 +subnet 才能让 geoip 用客户端 IP）：
dig @127.0.0.1 -p 5300 probe.geo.example.com TXT +subnet=174.192.0.1/32 +short
# → "co=us cn=na re=ct ci=windsor is=verizon wireless dm=myvzw.com ct=cellular aso=cellco-part"

# 验证 service 路由：
dig @127.0.0.1 -p 5300 www.geo.example.com A +subnet=14.215.176.1/32 +short
```

---

## 8. Lua records 使用新枚举

在 zone 里写 LUA 记录时，第二参数用 `GeoIPQueryAttribute.X`：

```lua
www.example.com  IN  LUA  A  "(function()
    local ip  = bestwho:toString()
    local co  = geoiplookup(ip, GeoIPQueryAttribute.Country) or ''
    local re  = geoiplookup(ip, GeoIPQueryAttribute.Region)  or ''
    local isp = geoiplookup(ip, GeoIPQueryAttribute.ISP)     or ''
    if co == 'cn' and re == 'gd' then return '1.1.1.1' end
    if co == 'cn'                 then return '2.2.2.2' end
    return '8.8.8.8'
end)()"
```

可用枚举：`ASn / City / Continent / Country / Country2 / Name / Region / Location /
Domain / ISP / ASO / ORG / ASN2 / ConnectionType`。

**注意**：
- `geoiplookup` 返回值是 **lowercased**，比较时用小写。
- PowerDNS 自动在 LUA `content` 前补 `return ` 前缀，因此完整语句必须包在 IIFE
  `(function() ... end)()` 里。

---

## 9. 常见问题

| 现象 | 排查 |
|------|------|
| `dig +subnet` 全返回 `unknown` | 检查 `edns-subnet-processing=yes`；没开就用 resolver IP(127.0.0.1)查 |
| 启动失败 *No backends configured* | `--config-dir` 没找到 `pdns.conf` |
| `%aso=cellco-parto` 多了个 `o` | 旧版 bug，已在 `geoipbackend: fix %aso placeholder being shadowed by %as` 修复，确认是本分支最新 commit |
| `re=unknown` / `ci=unknown` 比例高 | MaxMind 数据本身对该 IP 没有子分区信息；不是 bug |
| LUA record 报 *unexpected symbol near 'return'* | body 里有控制流，没用 IIFE 包；见上一节示例 |
| LUA record 报 *Unable to convert ... GeoIPQueryAttribute* | 第二参数写成了字符串 `"Country"`；应改为 `GeoIPQueryAttribute.Country` |

---

## 10. systemd 单元（可选）

```ini
# /etc/systemd/system/pdns.service
[Unit]
Description=PowerDNS Authoritative Server (province routing fork)
After=network.target mariadb.service

[Service]
Type=simple
ExecStart=/usr/local/powerdns/sbin/pdns_server --config-dir=/etc/pdns --daemon=no --guardian=no
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

```bash
systemctl daemon-reload
systemctl enable --now pdns
```

---

## 11. 回到上游

本分支与 upstream/master 的差异只集中在 `modules/geoipbackend/*` 与
`pdns/lua-record.cc` 的少数行；定期同步上游：

```bash
git remote add upstream https://github.com/PowerDNS/pdns.git  # 如未加
git fetch upstream
git merge upstream/master   # 或 rebase
```
