const fs = require('fs');
let code = fs.readFileSync('src/components/Skills.tsx', 'utf8');

const regexProgramming = /skills: 'C, C\+\+, Python, JavaScript, SQL, x86-64 Assembly'/;
code = code.replace(regexProgramming, "skills: 'C, C++, Java, Python, TypeScript, JavaScript, SQL, x86-64 Assembly'");

const regexBackend = /skills: 'Node\.js, Express\.js, REST APIs, PostgreSQL, MongoDB, React\.js'/;
code = code.replace(regexBackend, "skills: 'Java Spring Boot, Node.js, Express.js, REST APIs, PostgreSQL, MongoDB, React.js, HTML/CSS'");

const regexTools = /skills: 'Git, Make, Bash'/;
code = code.replace(regexTools, "skills: 'Git, Docker, AWS, Make, Bash'");

fs.writeFileSync('src/components/Skills.tsx', code);
