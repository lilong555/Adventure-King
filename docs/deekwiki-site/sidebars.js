// @ts-check

// This runs in Node.js - Don't use client-side code here (browser APIs, JSX...)

/**
 * Creating a sidebar enables you to:
 - create an ordered group of docs
 - render a sidebar for each doc of that group
 - provide next/previous navigation

 The sidebars can be generated from the filesystem, or explicitly defined here.

 Create as many sidebars as you want.

 @type {import('@docusaurus/plugin-content-docs').SidebarsConfig}
 */
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';

function parseReadmeOrder(readmeContent) {
  const items = [];
  for (const line of readmeContent.split(/\r?\n/)) {
    const match = line.match(/^\s*-\s*\[[^\]]+\]\(([^)]+\.md)\)\s*$/);
    if (match) {
      items.push(match[1]);
    }
  }
  return items;
}

function docIdFromFilename(filename) {
  return filename.replace(/\.md$/i, '');
}

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const docsRoot = path.resolve(__dirname, '..', 'deekwiki');
const readmePath = path.join(docsRoot, 'README.md');
const readmeContent = fs.readFileSync(readmePath, 'utf8');
const docFilesInOrder = parseReadmeOrder(readmeContent);
const docIdsInOrder = docFilesInOrder.map(docIdFromFilename);

const sidebars = {
  deekwikiSidebar: ['README', ...docIdsInOrder],

  // But you can create a sidebar manually
  /*
  tutorialSidebar: [
    'intro',
    'hello',
    {
      type: 'category',
      label: 'Tutorial',
      items: ['tutorial-basics/create-a-document'],
    },
  ],
   */
};

export default sidebars;
