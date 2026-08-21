const fs = require('fs');
let html = fs.readFileSync('index.html', 'utf8');

const regex = /<link rel="icon" type="image\/svg\+xml" href="data:image\/svg\+xml,.*?" \/>/;
const newFavicon = `<link rel="icon" type="image/svg+xml" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'%3E%3Ccircle cx='50' cy='50' r='50' fill='%23000000' /%3E%3Ctext x='50%25' y='50%25' text-anchor='middle' dy='.34em' font-family='-apple-system, BlinkMacSystemFont, sans-serif' font-size='46' font-weight='400' fill='%23ffffff'%3ER%3C/text%3E%3C/svg%3E" />`;

html = html.replace(regex, newFavicon);
fs.writeFileSync('index.html', html);
