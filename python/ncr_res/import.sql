-- !AI生成，未测试
-- merge_template.sql
-- 合并另一个 SQLite 数据库到当前数据库
-- 用法：在 SQLite 命令行执行，或通过 Python executescript()
-- 将 {{source_path}} 替换为实际路径

-- 关闭外键约束加速插入（合并后再开启）
PRAGMA foreign_keys = OFF;

-- 附加源数据库
ATTACH DATABASE 'OLD_SAVEDATA.db' AS src;

-- 合并用户（若有冲突则跳过）
INSERT OR IGNORE INTO users (uuid, name, passwd, note, time, type)
SELECT uuid, name, passwd, note, time, type FROM src.users;

-- 合并消息（处理孤儿消息：若 owner 不存在则设为 SYSTEM 用户）
INSERT OR IGNORE INTO messages (uuid, owner, time, content, type)
SELECT 
    m.uuid,
    COALESCE(
        (SELECT uuid FROM users WHERE uuid = m.owner),
        '7d87fb06-64c9-45bc-8b24-397c60d6001b'  -- SYSTEM 的 UUID
    ) AS owner,
    m.time,
    m.content,
    m.type
FROM src.messages m;

-- 合并编辑历史（只合并消息存在的记录）
INSERT OR IGNORE INTO edit_hist (mid, time, diff)
SELECT e.mid, e.time, e.diff
FROM src.edit_hist e
WHERE EXISTS (SELECT 1 FROM messages WHERE uuid = e.mid);

-- 合并标签（按 tag 去重）
INSERT OR IGNORE INTO tags (tag)
SELECT tag FROM src.tags;

-- 合并消息-标签关联（只关联存在的消息和标签）
INSERT OR IGNORE INTO msgs_tags (mid, tid)
SELECT mt.mid, mt.tid
FROM src.msgs_tags mt
WHERE EXISTS (SELECT 1 FROM messages WHERE uuid = mt.mid)
  AND EXISTS (SELECT 1 FROM tags WHERE id = mt.tid);

-- 合并事件（可选）
INSERT OR IGNORE INTO event (uid, time, type)
SELECT uid, time, type FROM src.event
WHERE uid IS NULL OR EXISTS (SELECT 1 FROM users WHERE uuid = uid);

-- 清理
DETACH DATABASE src;

-- 重新开启外键约束
PRAGMA foreign_keys = ON;
