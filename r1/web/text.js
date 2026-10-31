'use strict';
// Textos de la interfaz en ingles y espanol. Los nombres (Pokemon, ataques,
// objetos, rutas) los da el motor ya en el idioma activo.

const LANG_ES = 0, LANG_EN = 1;
let lang = LANG_EN;

const TXT = {
  // arranque
  chooseLang: ['IDIOMA', 'LANGUAGE'],
  chooseStarter: ['ELIGE TU COMPAÑERO', 'CHOOSE YOUR PARTNER'],
  starterNote: ['Vivirá en tu Poké Ball toda la aventura.', 'It will live in your Poké Ball for the whole adventure.'],
  // inicio
  explore: ['Explorar', 'Explore'],
  pokemon: ['Pokémon', 'Pokémon'],
  bag: ['Mochila', 'Bag'],
  shop: ['Tiendas', 'Shops'],
  gyms: ['Gimnasios', 'Gyms'],
  tournaments: ['Torneos', 'Tournaments'],
  pokedex: ['Pokédex', 'Pokédex'],
  settings: ['Ajustes', 'Settings'],
  day: ['Día', 'Day'],
  night: ['Noche', 'Night'],
  // explorar
  outsideEvents: ['Eventos afuera', 'Outside events'],
  searchRoute: ['Buscar en una ruta', 'Search a route'],
  incense: ['Incienso', 'Incense'],
  eggs: ['Huevos', 'Eggs'],
  center: ['Centro Pokémon', 'Pokémon Center'],
  walking: ['Caminando', 'Walking'],
  outside: ['Afuera', 'Outside'],
  atHome: ['En casa', 'At home'],
  testSteps: ['Pasos de prueba +500', 'Test steps +500'],
  noEvents: ['No hay eventos. Camina afuera: uno cada 400 pasos.', 'No events. Walk outside: one every 400 steps.'],
  needEE: ['Hace falta 300 EE. Camina para llenar el banco.', 'You need 300 EE. Walk to fill the bank.'],
  routeLocked: ['Ruta cerrada: faltan medallas.', 'Route locked: you need more badges.'],
  badgesN: ['medallas', 'badges'],
  afterChampion: ['Tras ser Campeón', 'After Champion'],
  incenseOn: ['Incienso activo', 'Incense active'],
  incenseNeed: ['No tienes Incienso.', 'You have no Incense.'],
  incenseStarted: ['¡Incienso encendido! 30 min, un Pokémon cada 5 min.', 'Incense lit! 30 min, one Pokémon every 5 min.'],
  healHome: ['¡Tu equipo está curado!', 'Your team is fully healed!'],
  healAway: ['Fuera de casa: tu equipo estará listo en 30 min.', 'Away from home: your team will be ready in 30 min.'],
  healing: ['Curando', 'Healing'],
  noEggs: ['No llevas huevos. Salen de PokéStops y Supply Drops.', 'No eggs. They come from PokéStops and Supply Drops.'],
  eggReady: ['¡Listo!', 'Ready!'],
  hatched: ['¡Nació {0}!', '{0} hatched!'],
  eggTier: [['', 'Común', 'Raro', 'Muy raro'], ['', 'Common', 'Rare', 'Very rare']],
  // eventos
  wildAppeared: ['¡Un {0} salvaje!', 'A wild {0} appeared!'],
  legendAppeared: ['¡{0} apareció!', '{0} appeared!'],
  trainerWants: ['¡{0} quiere luchar!', '{0} wants to battle!'],
  foundItem: ['¡Encontraste {1} {0}!', 'You found {1} {0}!'],
  foundNothing: ['No pasó nada...', 'Nothing happened...'],
  supplyDrop: ['¡Supply Drop!', 'Supply Drop!'],
  gotItems: ['Recibiste:', 'You got:'],
  berries: ['Bayas', 'Berries'],
  gotEgg: ['¡Un huevo {0}!', 'A {0} egg!'],
  merchant: ['¡Un Mercader Ambulante!', 'A Traveling Merchant!'],
  buy: ['Comprar', 'Buy'],
  leave: ['Irse', 'Leave'],
  battleIt: ['Luchar', 'Battle'],
  throwBall: ['Lanzar bola', 'Throw ball'],
  run: ['Huir', 'Run'],
  decline: ['Rechazar', 'Decline'],
  // lanzamiento
  aimHint: ['Inclina para apuntar · gira la rueda hacia arriba para lanzar', 'Tilt to aim · flick the wheel up to throw'],
  aimHintDesk: ['Ratón para apuntar · rueda arriba o espacio para lanzar', 'Mouse to aim · wheel up or space to throw'],
  missed: ['¡Falló! La bola se perdió.', 'Missed! The ball is lost.'],
  short: ['Muy suave: la bola se quedó corta.', 'Too soft: the ball fell short.'],
  caught: ['¡Atrapado! {0} es tuyo.', 'Gotcha! {0} was caught!'],
  brokeFree: ['¡Oh no! Se escapó.', 'Oh no! It broke free!'],
  fled: ['{0} huyó...', '{0} fled...'],
  noBalls: ['No tienes Poké Balls.', 'You have no Poké Balls.'],
  boxFull: ['El equipo y el PC están llenos.', 'Your party and PC are full.'],
  chooseBall: ['¿Qué bola?', 'Which ball?'],
  throwNames: [['', 'Bien', 'Genial', 'Excelente'], ['', 'Nice', 'Great', 'Excellent']],
  // combate
  fight: ['LUCHA', 'FIGHT'],
  bagUp: ['MOCHILA', 'BAG'],
  pkmnUp: ['POKéMON', 'POKéMON'],
  runUp: ['HUIR', 'RUN'],
  whatWill: ['¿Qué hará {0}?', 'What will {0} do?'],
  noPP: ['¡No quedan PP!', 'No PP left!'],
  cantRun: ['¡No puedes huir de un combate contra un entrenador!', 'You can\'t run from a trainer battle!'],
  chooseNext: ['Elige el siguiente Pokémon.', 'Choose your next Pokémon.'],
  fainted: ['debilitado', 'fainted'],
  alreadyOut: ['¡Ya está en combate!', 'It\'s already out!'],
  noItemsLeft: ['Ya no puedes usar objetos en este combate.', 'No more items allowed this battle.'],
  noEffect: ['No tendría efecto.', 'It won\'t have any effect.'],
  youWon: ['¡Ganaste!', 'You won!'],
  youLost: ['Perdiste...', 'You lost...'],
  gotMoney: ['Ganaste ₽{0}.', 'You got ₽{0}.'],
  lostMoney: ['Perdiste ₽{0} y corriste al Centro Pokémon.', 'You lost ₽{0} and hurried to a Pokémon Center.'],
  blackoutNoMoney: ['Tu equipo se recupera en el Centro Pokémon.', 'Your team recovers at the Pokémon Center.'],
  gotBadge: ['¡Medalla de {0}!', '{0}\'s badge!'],
  gotTM: ['Recibiste la MT{0}.', 'You got TM{0}.'],
  gotPrize: ['Recibiste {1} {0}.', 'You got {1} {0}.'],
  champion: ['¡ERES EL CAMPEÓN!', 'YOU ARE THE CHAMPION!'],
  // aprender / evolucionar
  wantsLearn: ['{0} quiere aprender {1}. ¿Cuál olvida?', '{0} wants to learn {1}. Forget which move?'],
  dontLearn: ['No aprender', 'Don\'t learn'],
  learned: ['¡{0} aprendió {1}!', '{0} learned {1}!'],
  evolving: ['¿Qué? ¡{0} está evolucionando!', 'What? {0} is evolving!'],
  evolveQ: ['¿Dejar que evolucione?', 'Let it evolve?'],
  evolved: ['¡{0} evolucionó a {1}!', '{0} evolved into {1}!'],
  stopped: ['{0} no evolucionó.', '{0} did not evolve.'],
  yes: ['Sí', 'Yes'],
  no: ['No', 'No'],
  // equipo
  summary: ['Datos', 'Summary'],
  swap: ['Mover', 'Swap'],
  useItem: ['Usar objeto', 'Use item'],
  teachTM: ['Enseñar MT', 'Teach TM'],
  toPC: ['Dejar en el PC', 'Deposit to PC'],
  pc: ['PC', 'PC'],
  withdraw: ['Sacar', 'Withdraw'],
  partnerStays: ['Tu compañero no se puede dejar.', 'Your partner can\'t be deposited.'],
  partyFull: ['El equipo está lleno.', 'Your party is full.'],
  lastOne: ['Te quedarías sin equipo.', 'You\'d have no team left.'],
  swapWith: ['¿Con cuál lo cambias?', 'Swap with which?'],
  trust: ['Confianza', 'Trust'],
  exp: ['EXP', 'EXP'],
  stats: ['ATQ DEF VEL ESP', 'ATK DEF SPE SPC'],
  cantLearn: ['No puede aprender esa MT.', 'It can\'t learn that TM.'],
  forgetWhich: ['¿Qué movimiento olvida?', 'Forget which move?'],
  // mochila
  pockets: [['Objetos', 'Bolas', 'Medicina', 'Clave', 'MT'], ['Items', 'Balls', 'Medicine', 'Key', 'TMs']],
  empty: ['(vacío)', '(empty)'],
  onWhich: ['¿En cuál?', 'On which?'],
  used: ['¡Usaste {0}!', 'Used {0}!'],
  grewTo: ['¡{0} subió al nivel {1}!', '{0} grew to Lv{1}!'],
  // tiendas
  mart: ['Poké Mart', 'Poké Mart'],
  celadon: ['Celadon Dept. Store', 'Celadon Dept. Store'],
  battleShop: ['Battle Shop', 'Battle Shop'],
  sell: ['Vender', 'Sell'],
  celadonClosed: ['Abre con la medalla de Erika.', 'Opens with Erika\'s badge.'],
  howMany: ['¿Cuántos?', 'How many?'],
  bought: ['¡Comprado!', 'Bought!'],
  sold: ['¡Vendido!', 'Sold!'],
  shopErr: [['', 'No se vende.', 'No te alcanza el dinero.', 'No te alcanzan los BP.', 'Ya llegaste al tope.', 'La mochila está llena.', 'Cerrado.'],
            ['', 'Not sold here.', 'Not enough money.', 'Not enough BP.', 'Limit reached.', 'Your bag is full.', 'Closed.']],
  // gimnasios
  league: ['Liga Pokémon', 'Pokémon League'],
  locked: ['Cerrado', 'Locked'],
  kmToOpen: ['{0} km para abrir', '{0} km to open'],
  needBadge: ['Falta la medalla anterior', 'Needs the previous badge'],
  challenge: ['Retar', 'Challenge'],
  leagueNeed: ['8 medallas y 80 km', '8 badges and 80 km'],
  leagueNext: ['Siguiente: {0}', 'Next: {0}'],
  leagueQuit: ['Abandonar la Liga', 'Quit the League'],
  // torneos
  daily: ['Daily Challenge', 'Daily Challenge'],
  dailyDone: ['Ya jugaste hoy.', 'Already played today.'],
  featured: ['Destacada', 'Featured'],
  pickTeam: ['Elige 3 ({0}/3)', 'Pick 3 ({0}/3)'],
  fee: ['Cuota', 'Fee'],
  tourErr: [['', 'Elección no válida.', 'Repetido.', 'No cumple la regla de la copa.', 'No te alcanza el dinero.', 'Cerrado.', 'Ya jugaste el Daily hoy.', 'Necesitas 3 Pokémon en el PC.'],
            ['', 'Invalid pick.', 'Duplicate.', 'Doesn\'t meet the cup rule.', 'Not enough money.', 'Locked.', 'Daily already played today.', 'You need 3 Pokémon in the PC.']],
  round: [['Primera ronda', 'Semifinal', 'Final'], ['First round', 'Semi-final', 'Final']],
  tourChamp: ['¡Campeón del torneo!', 'Tournament champion!'],
  tourFinal: ['Finalista.', 'Finalist.'],
  tourOut: ['Eliminado.', 'Knocked out.'],
  perfect: ['¡Perfect Run!', 'Perfect Run!'],
  gotBP: ['+{0} BP', '+{0} BP'],
  weekly: ['Premio semanal: {0}', 'Weekly prize: {0}'],
  dailyRule: [['Sin regla', 'Monotipo: {0}', 'Sin objetos', 'Copa Pequeña'], ['No rule', 'Mono-Type: {0}', 'No items', 'Little Cup']],
  dailyPrize: ['Premio: {0}', 'Prize: {0}'],
  continueTour: ['Seguir el torneo', 'Continue tournament'],
  // ajustes
  language: ['Idioma', 'Language'],
  resetGame: ['Borrar partida', 'Delete save'],
  resetQ: ['¿Borrar la partida? No se puede deshacer.', 'Delete your save? This can\'t be undone.'],
  invertAim: ['Invertir apuntado', 'Invert aim'],
  credits: ['Créditos', 'Credits'],
  creditsText: ['Sprites: PMD SpriteCollab (CC BY-NC 4.0). Fuente: Press Start 2P (OFL). Pokémon © Nintendo / Game Freak / The Pokémon Company. Proyecto personal sin fines comerciales.',
                'Sprites: PMD SpriteCollab (CC BY-NC 4.0). Font: Press Start 2P (OFL). Pokémon © Nintendo / Game Freak / The Pokémon Company. Personal, non-commercial project.'],
  back: ['Volver', 'Back'],
  on: ['Sí', 'On'],
  off: ['No', 'Off'],
  saved: ['Guardado', 'Saved'],
  loading: ['Cargando...', 'Loading...'],
};

function L(key, ...args) {
  const v = TXT[key];
  if (!v) return key;
  let s = v[lang === LANG_ES ? 0 : 1];
  if (typeof s !== 'string') return s;
  args.forEach((a, i) => { s = s.split('{' + i + '}').join(a); });
  return s;
}

// ---------------------------------------------------------------- combate --

const STAT_NAMES = [
  ['', 'ATAQUE', 'DEFENSA', 'VELOCIDAD', 'ESPECIAL', 'PRECISIÓN', 'EVASIÓN'],
  ['', 'ATTACK', 'DEFENSE', 'SPEED', 'SPECIAL', 'ACCURACY', 'EVASION'],
];
const MV = { MIST: 54, MIMIC: 102, LIGHT_SCREEN: 113, HAZE: 114, REFLECT: 115, FOCUS_ENERGY: 116,
             TRANSFORM: 144, CONVERSION: 160, SUBSTITUTE: 164 };
const AIL = { CONFUSION: 2, DISABLE: 3, LEECH_SEED: 5, TRAP: 9 };

// nombre del Pokemon en el texto: el rival lleva "salvaje" o "rival"
function who(ev, wild) {
  if (ev.s === 0) return ev.n;
  if (lang === LANG_ES) return ev.n + (wild ? ' salvaje' : ' rival');
  return (wild ? 'Wild ' : 'Foe ') + ev.n;
}

// texto de un evento del motor (null = sin texto, solo animacion)
function eventText(ev, ctx) {
  const es = lang === LANG_ES;
  const w = who(ev, ctx.wild);
  switch (ev.k) {
    case 'move': return es ? `¡${w} usó ${ev.m}!` : `${w} used ${ev.m}!`;
    case 'miss': return es ? `¡El ataque de ${w} falló!` : `${w}'s attack missed!`;
    case 'crit': return es ? '¡Un golpe crítico!' : 'A critical hit!';
    case 'eff':
      if (ev.a === 0xF0) return es ? '¡KO de un golpe!' : 'It\'s a one-hit KO!';
      if (ev.a === 0xF1) return es ? `No afecta a ${w}...` : `It doesn't affect ${w}...`;
      if (ev.a === 0xF2) return ev.b > 2 ? (es ? '¡Es súper eficaz!' : 'It\'s super effective!')
                                         : (es ? 'No es muy eficaz...' : 'It\'s not very effective...');
      if (ev.a === 0xF3) return es ? '¡Escapaste sin problemas!' : 'Got away safely!';
      if (ev.a === AIL.TRAP) return es ? `¡${w} quedó atrapado!` : `${w} is trapped!`;
      if (ev.a === AIL.LEECH_SEED) return es ? `¡${w} fue infectado!` : `${w} was seeded!`;
      if (ev.a === AIL.DISABLE) return es ? `¡${ev.m} de ${w} quedó anulado!` : `${w}'s ${ev.m} was disabled!`;
      if (ev.a === AIL.CONFUSION) return es ? `${w} ya no está confuso.` : `${w}'s confused no more!`;
      if (ev.a === MV.FOCUS_ENERGY) return es ? `¡${w} se concentra!` : `${w} is getting pumped!`;
      if (ev.a === MV.REFLECT || ev.a === MV.LIGHT_SCREEN || ev.a === MV.MIST)
        return es ? `¡${w} se protege con ${ev.m}!` : `${w} is protected by ${ev.m}!`;
      if (ev.a === MV.HAZE) return es ? '¡Se anularon los cambios de stats!' : 'All stat changes were eliminated!';
      if (ev.a === MV.SUBSTITUTE) return es ? `¡${w} creó un sustituto!` : `${w} made a substitute!`;
      if (ev.a === MV.CONVERSION) return es ? `¡${w} cambió de tipo!` : `${w} changed its type!`;
      if (ev.a === MV.TRANSFORM) return es ? `¡${w} se transformó!` : `${w} transformed!`;
      if (ev.a === MV.MIMIC) return es ? `¡${w} copió un movimiento!` : `${w} copied a move!`;
      return null;
    case 'status': {
      const S = es ? ['', 'está paralizado', 'se durmió', 'se congeló', 'se quemó', 'fue envenenado', 'fue gravemente envenenado']
                   : ['', 'is paralyzed', 'fell asleep', 'was frozen solid', 'was burned', 'was poisoned', 'was badly poisoned'];
      if (ev.a === 0x80) return es ? `¡${w} está confuso!` : `${w} became confused!`;
      return S[ev.a] ? `¡${w} ${S[ev.a]}!`.replace(/^¡/, es ? '¡' : '') : null;
    }
    case 'stat': {
      const name = STAT_NAMES[es ? 0 : 1][ev.a] || '';
      if (ev.b === 0) return es ? `¡${name} de ${w} no cambia más!` : `${w}'s ${name} won't go any further!`;
      const up = ev.b > 0, big = Math.abs(ev.b) > 1;
      if (es) return `¡${up ? 'Subió' : 'Bajó'} ${big ? 'mucho ' : ''}el ${name} de ${w}!`;
      return `${w}'s ${name} ${up ? (big ? 'sharply rose' : 'rose') : (big ? 'harshly fell' : 'fell')}!`;
    }
    case 'heal': return es ? `¡${w} recuperó salud!` : `${w} regained health!`;
    case 'faint': return es ? `¡${w} se debilitó!` : `${w} fainted!`;
    case 'switch':
      if (ev.s === 0) return es ? `¡Adelante, ${ev.n}!` : `Go! ${ev.n}!`;
      return es ? `¡${ctx.foeName || 'El rival'} sacó a ${ev.n}!` : `${ctx.foeName || 'Foe'} sent out ${ev.n}!`;
    case 'fail': return es ? '¡Pero falló!' : 'But it failed!';
    case 'flinch': return es ? `¡${w} retrocedió!` : `${w} flinched!`;
    case 'confhit': return es ? '¡Está tan confuso que se hirió!' : 'It hurt itself in its confusion!';
    case 'fullpara': return es ? `¡${w} está paralizado y no se mueve!` : `${w} is fully paralyzed!`;
    case 'asleep': return es ? `${w} está dormido.` : `${w} is fast asleep.`;
    case 'wake': return es ? `¡${w} se despertó!` : `${w} woke up!`;
    case 'frozen': return es ? `¡${w} está congelado!` : `${w} is frozen solid!`;
    case 'thaw': return es ? `¡${w} se descongeló!` : `${w} thawed out!`;
    case 'recharge': return es ? `${w} necesita recuperarse.` : `${w} must recharge!`;
    case 'charge': return es ? `${w} prepara ${ev.m}...` : `${w} is charging ${ev.m}...`;
    case 'disobey': {
      const D = es ? ['no hace caso', 'hace otra cosa', 'se echó una siesta', 'se hizo daño']
                   : ['is loafing around', 'won\'t obey', 'began to nap', 'hurt itself'];
      return `${w} ${D[ev.a] || D[0]}!`;
    }
    case 'item': return es ? `¡${ctx.foeName || 'El rival'} usó una poción!` : `${ctx.foeName || 'Foe'} used a Potion!`;
    case 'exp': return es ? `¡${ev.n} ganó ${ev.v} EXP!` : `${ev.n} gained ${ev.v} EXP!`;
    case 'lvl': return es ? `¡${ev.n} subió al nivel ${ev.lv}!` : `${ev.n} grew to Lv${ev.lv}!`;
    case 'useitem': return L('used', ev.item);
  }
  return null;
}
