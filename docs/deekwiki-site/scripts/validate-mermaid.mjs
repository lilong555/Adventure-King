import fs from 'node:fs';
import path from 'node:path';
import DOMPurify from 'dompurify';

if (typeof DOMPurify.addHook !== 'function') {
  DOMPurify.addHook = () => {};
}
if (typeof DOMPurify.removeHook !== 'function') {
  DOMPurify.removeHook = () => {};
}
if (typeof DOMPurify.sanitize !== 'function') {
  DOMPurify.sanitize = (value) => value;
}

const {default: mermaid} = await import('mermaid');

function listFilesRecursive(rootDir) {
  const out = [];
  const queue = [rootDir];
  while (queue.length) {
    const dir = queue.pop();
    const entries = fs.readdirSync(dir, {withFileTypes: true});
    for (const entry of entries) {
      const fullPath = path.join(dir, entry.name);
      if (entry.isDirectory()) {
        queue.push(fullPath);
      } else {
        out.push(fullPath);
      }
    }
  }
  return out;
}

function extractMermaidBlocks(markdown) {
  const blocks = [];
  const lines = markdown.split(/\r?\n/);
  let inBlock = false;
  let startLine = 0;
  let buf = [];
  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];
    if (!inBlock) {
      if (line.trim() === '```mermaid') {
        inBlock = true;
        startLine = i + 1;
        buf = [];
      }
      continue;
    }

    if (line.trim() === '```') {
      blocks.push({startLine, text: buf.join('\n')});
      inBlock = false;
      buf = [];
      continue;
    }

    buf.push(line);
  }

  if (inBlock) {
    blocks.push({startLine, text: buf.join('\n'), unterminated: true});
  }

  return blocks;
}

const siteRoot = path.resolve(import.meta.dirname, '..');
const docsRoot = path.resolve(siteRoot, '..', 'deekwiki');
const mdFiles = listFilesRecursive(docsRoot).filter((p) => p.toLowerCase().endsWith('.md'));

let hasError = false;

for (const filePath of mdFiles) {
  const rel = path.relative(siteRoot, filePath);
  const content = fs.readFileSync(filePath, 'utf8');
  const blocks = extractMermaidBlocks(content);
  for (let index = 0; index < blocks.length; index++) {
    const block = blocks[index];
    if (block.unterminated) {
      hasError = true;
      console.error(`[mermaid] 未闭合代码块: ${rel}:${block.startLine}`);
      continue;
    }
    try {
      await mermaid.parse(block.text);
    } catch (err) {
      hasError = true;
      const message = err instanceof Error ? err.message : String(err);
      console.error(`[mermaid] 解析失败: ${rel}:${block.startLine} (block #${index + 1})`);
      console.error(message.trimEnd());
      console.error('---');
    }
  }
}

if (hasError) {
  process.exitCode = 1;
} else {
  console.log(`[mermaid] OK (${mdFiles.length} 个 Markdown 文件)`);
}
