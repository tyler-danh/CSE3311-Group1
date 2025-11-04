function Logo() {
  return (
    <div className="logo">
      <svg width="80" height="80" viewBox="0 0 100 100">
        <path
          d="M20 80 L50 20 L50 80 Z"
          fill="#5fb893"
          stroke="#ffffff"
          strokeWidth="3"
        />
        <path
          d="M50 20 L80 80 L50 80 Z"
          fill="none"
          stroke="#ffffff"
          strokeWidth="3"
          strokeDasharray="5,5"
        />
      </svg>
      <h1>stegaSaur</h1>
    </div>
  )
}

export default Logo
